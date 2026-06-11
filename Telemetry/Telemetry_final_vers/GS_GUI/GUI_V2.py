#!/usr/bin/env python3
"""
SOAR Ground Station GUI
========================
Reads serial output from the GS radio module and displays live telemetry.


Parses GS serial lines of the form:
    [TLM ALT] bay_seq=X gs_seq=Y Data: 1 HH:MM:SS.mmm,alt,temp,pressure
    [TLM IMU] bay_seq=X gs_seq=Y Data: 0 HH:MM:SS.mmm,ax,...,wz
    [TLM GPS] bay_seq=X gs_seq=Y Data: 2 HH:MM:SS.mmm,nmea...
    [TLM KAL] bay_seq=X gs_seq=Y Data: 3 HH:MM:SS.mmm, state, alt,vel,acc
    RSSI: -XX


Commands sent to GS (which forwards over radio to Bay):
    ping             -> PING,<seq>
    reboot           -> REBOOT,<seq>
    freq <MHz>       -> FREQ,<seq>,<MHz>
    <anything else>  -> sent raw


Dependencies:
    pip install pyserial pyqt6 pyqtgraph
"""


import sys
import re
import serial
import serial.tools.list_ports
from collections import deque
from threading import Thread


from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QComboBox, QLineEdit, QLabel, QTextEdit, QSplitter,
    QTabWidget, QGroupBox, QFrame, QScrollArea,
)
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QObject
from PyQt6.QtGui import QFont, QColor, QPalette
import pyqtgraph as pg


# ──────────────────────────────────────────────────────────────────────────────
# Constants
# ──────────────────────────────────────────────────────────────────────────────
MAX_POINTS  = 600   # rolling window
PLOT_HZ     = 10    # plot refresh rate


PALETTE = {
    'bg':        '#0e0e1c',
    'panel':     '#16162a',
    'border':    '#2a2a4a',
    'accent':    '#5555ff',
    'text':      '#d0d0e0',
    'dim':       '#666688',


    'red':    '#ff4444',
    'green':  '#44ff88',
    'yellow': '#ffd700',
    'cyan':   '#00ddff',
    'orange': '#ff8833',
    'purple': '#cc55ff',
    'blue':   '#4488ff',
    'teal':   '#33ddaa',
    'pink':   '#ff55aa',
    'lime':   '#aaff44',
}


STYLESHEET = f"""
QMainWindow, QWidget {{
    background-color: {PALETTE['bg']};
    color: {PALETTE['text']};
    font-family: 'Segoe UI', 'Arial', sans-serif;
    font-size: 12px;
}}
QGroupBox {{
    border: 1px solid {PALETTE['border']};
    border-radius: 5px;
    margin-top: 10px;
    padding-top: 6px;
    font-weight: bold;
    font-size: 11px;
}}
QGroupBox::title {{
    color: {PALETTE['accent']};
    subcontrol-origin: margin;
    left: 8px;
    top: -1px;
    padding: 0 4px;
}}
QPushButton {{
    background-color: {PALETTE['panel']};
    border: 1px solid {PALETTE['border']};
    border-radius: 4px;
    padding: 5px 12px;
    color: {PALETTE['text']};
    font-size: 12px;
}}
QPushButton:hover {{ background-color: #222244; border-color: {PALETTE['accent']}; }}
QPushButton:pressed {{ background-color: #0a0a1a; }}
QPushButton#btn_connect {{
    background-color: #0e3322;
    border-color: #1a7744;
    color: {PALETTE['green']};
    font-weight: bold;
}}
QPushButton#btn_connect:hover {{ background-color: #1a4433; }}
QPushButton#btn_disconnect {{
    background-color: #33100e;
    border-color: #883322;
    color: {PALETTE['red']};
    font-weight: bold;
}}
QPushButton#btn_disconnect:hover {{ background-color: #441a1a; }}
QComboBox, QLineEdit {{
    background-color: {PALETTE['panel']};
    border: 1px solid {PALETTE['border']};
    border-radius: 4px;
    padding: 4px 6px;
    color: {PALETTE['text']};
}}
QComboBox::drop-down {{ border: none; width: 18px; }}
QComboBox QAbstractItemView {{
    background-color: {PALETTE['panel']};
    border: 1px solid {PALETTE['border']};
    selection-background-color: {PALETTE['accent']};
}}
QTextEdit {{
    background-color: #080812;
    border: 1px solid {PALETTE['border']};
    border-radius: 4px;
    color: #a0ffb0;
    font-family: 'Courier New', 'Consolas', monospace;
    font-size: 11px;
}}
QTabWidget::pane {{
    border: 1px solid {PALETTE['border']};
    border-radius: 4px;
}}
QTabBar::tab {{
    background: {PALETTE['panel']};
    border: 1px solid {PALETTE['border']};
    border-bottom: none;
    padding: 6px 18px;
    margin-right: 2px;
    border-radius: 4px 4px 0 0;
    color: {PALETTE['dim']};
    font-size: 12px;
}}
QTabBar::tab:selected {{
    background: {PALETTE['bg']};
    color: {PALETTE['cyan']};
    border-bottom: 2px solid {PALETTE['cyan']};
    font-weight: bold;
}}
QScrollBar:vertical {{
    background: {PALETTE['panel']};
    width: 8px;
    border-radius: 4px;
}}
QScrollBar::handle:vertical {{
    background: {PALETTE['border']};
    border-radius: 4px;
    min-height: 20px;
}}
QSplitter::handle {{ background: {PALETTE['border']}; }}
"""




# ──────────────────────────────────────────────────────────────────────────────
# Data Store
# ──────────────────────────────────────────────────────────────────────────────
class TelemetryStore:
    """Thread-safe-ish ring-buffer store for all sensor data."""
    def __init__(self, n=MAX_POINTS):
        # Altimeter
        self.alt_t        = deque(maxlen=n)
        self.alt_altitude = deque(maxlen=n)
        self.alt_temp     = deque(maxlen=n)
        self.alt_pressure = deque(maxlen=n)


        # Kalman
        self.kal_t   = deque(maxlen=n)
        self.kal_state = deque(maxlen=n)
        self.kal_alt = deque(maxlen=n)
        self.kal_vel = deque(maxlen=n)
        self.kal_acc = deque(maxlen=n)


        # IMU
        self.imu_t  = deque(maxlen=n)
        self.imu_ax = deque(maxlen=n)
        self.imu_ay = deque(maxlen=n)
        self.imu_az = deque(maxlen=n)
        self.imu_lx = deque(maxlen=n)
        self.imu_ly = deque(maxlen=n)
        self.imu_lz = deque(maxlen=n)
        self.imu_gx = deque(maxlen=n)
        self.imu_gy = deque(maxlen=n)
        self.imu_gz = deque(maxlen=n)
        self.imu_wx = deque(maxlen=n)
        self.imu_wy = deque(maxlen=n)
        self.imu_wz = deque(maxlen=n)


        # GPS
        self.gps_history = deque(maxlen=50)


        # Latest snapshot for stat boxes
        self.latest = {}
        self.rssi = None
        self.packet_count = 0


STORE = TelemetryStore()




# ──────────────────────────────────────────────────────────────────────────────
# Parser
# ──────────────────────────────────────────────────────────────────────────────
_TLM_RE     = re.compile(r'\[TLM\s+\w+\].*?Data:\s*(.+)', re.IGNORECASE)
_TLM_HDR_RE = re.compile(r'^(?:Callsign:[^|]+\s*\|\s*)?TLM,\d+,\d+,(.+)$')


def _ts_to_sec(ts: str) -> float:
    """HH:MM:SS.mmm → float seconds since midnight."""
    try:
        h, m, s = ts.split(':')
        return int(h) * 3600 + int(m) * 60 + float(s)
    except Exception:
        return 0.0


IMU_PATTERN = re.compile(r'\[GS RX IMU\] bay_seq=\d+ gs_seq=\d+ ts=([^ ]+) accel\[(.*?),(.*?),(.*?)\] linear\[(.*?),(.*?),(.*?)\] gravity\[(.*?),(.*?),(.*?)\] quat\[(.*?),(.*?),(.*?),(.*?)\] gyro\[(.*?),(.*?),(.*?)\]')
ALT_PATTERN = re.compile(r'\[GS RX ALT\] bay_seq=\d+ gs_seq=\d+ ts=([^ ]+) altitude=(.*?) temp=(.*?) pressure=(.*)')
GPS_PATTERN = re.compile(r'\[GS RX GPS\] bay_seq=\d+ gs_seq=\d+ ts=([^ ]+) nmea=(.*)')
KAL_PATTERN = re.compile(r'\[GS RX KAL\] bay_seq=\d+ gs_seq=\d+ ts=([^ ]+) state=(\d+) altitude=(.*?) velocity=(.*?) acceleration=(.*)')
def parse_line(line: str):
    """
    Parses new GS print format:
    [GS RX <TYPE>] bay_seq=X gs_seq=Y ts=...
    """
    line = line.strip()
    if not line:
        return None


    # Handle RSSI (new format: RSSI=XX)
    if line.upper().startswith('RSSI='):
        try:
            return ('RSSI', int(line.split('=')[1].strip()))
        except Exception:
            return None


    # IMU (Type 0)
    match = IMU_PATTERN.search(line)
    if match:
        vals = match.groups()
        return ('IMU', {
            'ts': vals[0], 't': _ts_to_sec(vals[0]),
            'ax': float(vals[1]), 'ay': float(vals[2]), 'az': float(vals[3]),
            'lx': float(vals[4]), 'ly': float(vals[5]), 'lz': float(vals[6]),
            'gx': float(vals[7]), 'gy': float(vals[8]), 'gz': float(vals[9]),
            'qw': float(vals[10]), 'qx': float(vals[11]), 'qy': float(vals[12]), 'qz': float(vals[13]),
            'wx': float(vals[14]), 'wy': float(vals[15]), 'wz': float(vals[16]),
        })


    # ALT (Type 1)
    match = ALT_PATTERN.search(line)
    if match:
        vals = match.groups()
        return ('ALT', {
            'ts': vals[0], 't': _ts_to_sec(vals[0]),
            'altitude': float(vals[1]),
            'temp': float(vals[2]),
            'pressure': float(vals[3])
        })


    # GPS (Type 2)
    match = GPS_PATTERN.search(line)
    if match:
        vals = match.groups()
        return ('GPS', {'ts': vals[0], 'nmea': vals[1]})


    # KALMAN (Type 3)
    match = KAL_PATTERN.search(line)
    if match:
        vals = match.groups()
        return ('KALMAN', {
            'ts': vals[0], 't': _ts_to_sec(vals[0]),
            'state': int(vals[1]),
            'alt': float(vals[2]),
            'vel': float(vals[3]),
            'acc': float(vals[4])
        })


    return None




# ──────────────────────────────────────────────────────────────────────────────
# Serial Worker
# ──────────────────────────────────────────────────────────────────────────────
class SerialSignals(QObject):
    line_received      = pyqtSignal(str)
    connection_changed = pyqtSignal(bool, str)




class SerialWorker:
    def __init__(self, signals: SerialSignals):
        self.signals = signals
        self._ser    = None
        self._running= False
        self._thread = None


    def connect(self, port: str, baud: int):
        try:
            self._ser = serial.Serial(port, baud, timeout=1)
            self._running = True
            self._thread  = Thread(target=self._loop, daemon=True)
            self._thread.start()
            self.signals.connection_changed.emit(True, f"Connected  {port} @ {baud} baud")
        except Exception as e:
            self.signals.connection_changed.emit(False, str(e))


    def disconnect(self):
        self._running = False
        if self._ser and self._ser.is_open:
            try:
                self._ser.close()
            except Exception:
                pass
        self.signals.connection_changed.emit(False, "Disconnected")


    def send(self, text: str):
        if self._ser and self._ser.is_open:
            try:
                self._ser.write((text + '\n').encode('utf-8'))
            except Exception:
                pass


    def _loop(self):
        while self._running:
            try:
                if self._ser.in_waiting:
                    raw = self._ser.readline().decode('utf-8', errors='replace').strip()
                    if raw:
                        self.signals.line_received.emit(raw)
            except Exception:
                self._running = False
                self.signals.connection_changed.emit(False, "Serial error — disconnected")
                break




# ──────────────────────────────────────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────────────────────────────────────
def make_plot(title: str, ylabel: str, xlabel: str = "Time (s)") -> pg.PlotWidget:
    pg.setConfigOption('background', '#080812')
    pg.setConfigOption('foreground', '#aaaacc')
    pw = pg.PlotWidget()
    pw.setTitle(f"<span style='color:#aaaacc;font-size:11pt'>{title}</span>")
    pw.setLabel('left',   ylabel)
    pw.setLabel('bottom', xlabel)
    pw.showGrid(x=True, y=True, alpha=0.25)
    pw.addLegend(offset=(10, 5))
    pw.getAxis('left').setStyle(tickTextOffset=4)
    return pw




def stat_card(label: str, color: str) -> tuple:
    """Returns (QGroupBox, value_QLabel)."""
    box = QGroupBox(label)
    box.setStyleSheet(
        f"QGroupBox {{ border-color: {color}50; }}"
        f"QGroupBox::title {{ color: {color}; }}"
    )
    layout = QVBoxLayout(box)
    layout.setContentsMargins(4, 4, 4, 4)
    val = QLabel("---")
    val.setAlignment(Qt.AlignmentFlag.AlignCenter)
    val.setFont(QFont("Segoe UI", 16, QFont.Weight.Bold))
    val.setStyleSheet(f"color: {color};")
    layout.addWidget(val)
    return box, val




# ──────────────────────────────────────────────────────────────────────────────
# Main Window
# ──────────────────────────────────────────────────────────────────────────────
class SOARGroundStation(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SOAR Ground Station — Live Telemetry")
        self.setMinimumSize(1400, 900)
        self.setStyleSheet(STYLESHEET)


        self._connected = False
        self._signals   = SerialSignals()
        self._worker    = SerialWorker(self._signals)
        self._signals.line_received.connect(self._on_line)
        
        self._signals.connection_changed.connect(self._on_conn)


        self._build_ui()


        self._timer = QTimer(self)
        self._timer.timeout.connect(self._refresh_plots)
        self._timer.start(1000 // PLOT_HZ)


    # ── UI Construction ───────────────────────────────────────────────────────


    def _build_ui(self):
        root = QWidget()
        self.setCentralWidget(root)
        vbox = QVBoxLayout(root)
        vbox.setSpacing(5)
        vbox.setContentsMargins(8, 8, 8, 8)


        vbox.addWidget(self._build_topbar())


        splitter = QSplitter(Qt.Orientation.Vertical)
        splitter.addWidget(self._build_main_area())
        splitter.addWidget(self._build_terminal())
        splitter.setSizes([680, 220])
        splitter.setChildrenCollapsible(False)
        vbox.addWidget(splitter, 1)


    # ── Top bar ───────────────────────────────────────────────────────────────


    def _build_topbar(self) -> QWidget:
        bar = QWidget()
        bar.setMaximumHeight(44)
        bar.setStyleSheet(
            f"background-color: {PALETTE['panel']};"
            f"border: 1px solid {PALETTE['border']};"
            f"border-radius: 5px;"
        )
        h = QHBoxLayout(bar)
        h.setContentsMargins(10, 4, 10, 4)


        # Logo / title
        title = QLabel("SOAR GS")
        title.setStyleSheet(
            f"color: {PALETTE['cyan']}; font-size: 15px; font-weight: bold; letter-spacing: 2px;"
        )
        h.addWidget(title)


        h.addWidget(self._vdiv())


        # Port
        h.addWidget(QLabel("Port"))
        self.port_cb = QComboBox()
        self.port_cb.setMinimumWidth(130)
        h.addWidget(self.port_cb)


        refresh_btn = QPushButton("⟳")
        refresh_btn.setFixedWidth(28)
        refresh_btn.setToolTip("Refresh serial ports")
        refresh_btn.clicked.connect(self._refresh_ports)
        h.addWidget(refresh_btn)


        h.addWidget(self._vdiv())


        # Baud
        h.addWidget(QLabel("Baud"))
        self.baud_cb = QComboBox()
        self.baud_cb.addItems(["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"])
        self.baud_cb.setCurrentText("921600")
        self.baud_cb.setMinimumWidth(90)
        h.addWidget(self.baud_cb)


        h.addWidget(self._vdiv())


        # Connect button
        self.conn_btn = QPushButton("Connect")
        self.conn_btn.setObjectName("btn_connect")
        self.conn_btn.setMinimumWidth(100)
        self.conn_btn.clicked.connect(self._toggle_connect)
        h.addWidget(self.conn_btn)


        h.addWidget(self._vdiv())


        # Status dot + text
        self.status_lbl = QLabel("● Disconnected")
        self.status_lbl.setStyleSheet(f"color: {PALETTE['red']}; font-weight: bold;")
        h.addWidget(self.status_lbl)


        h.addStretch()


        # RSSI
        h.addWidget(QLabel("RSSI"))
        self.rssi_lbl = QLabel("--- dBm")
        self.rssi_lbl.setStyleSheet(f"color: {PALETTE['yellow']}; font-weight: bold; min-width: 70px;")
        h.addWidget(self.rssi_lbl)
       
        # RSSI
        h.addWidget(QLabel("State"))
        self.state_lbl = QLabel("---")
        self.state_lbl.setStyleSheet(f"color: {PALETTE['green']}; font-weight: bold; min-width: 70px;")
        h.addWidget(self.state_lbl)


        h.addWidget(self._vdiv())


        # Packet counter
        h.addWidget(QLabel("Packets"))
        self.pkt_lbl = QLabel("0")
        self.pkt_lbl.setStyleSheet(f"color: {PALETTE['cyan']}; font-weight: bold; min-width: 40px;")
        h.addWidget(self.pkt_lbl)


        self._refresh_ports()
        return bar


    # ── Tabs ──────────────────────────────────────────────────────────────────


    def _build_main_area(self) -> QTabWidget:
        tabs = QTabWidget()
        tabs.addTab(self._tab_altitude(),  "📡  Altitude / Kalman")
        tabs.addTab(self._tab_imu(),       "🔄  IMU")
        tabs.addTab(self._tab_gps(),       "🛰  GPS")
        return tabs


    # ── Altitude / Kalman tab ─────────────────────────────────────────────────


    def _tab_altitude(self) -> QWidget:
        w = QWidget()
        v = QVBoxLayout(w)
        v.setSpacing(6)


        # Stat cards row
        card_row = QHBoxLayout()
        self._stat = {}
        defs = [
            ('baro_alt',  "Baro Alt",    PALETTE['cyan'],   "{:.1f} m"),
            ('kal_alt',   "Kalman Alt",  PALETTE['yellow'],  "{:.1f} m"),
            ('kal_vel',   "Velocity",    PALETTE['orange'],  "{:.2f} m/s"),
            ('kal_acc',   "K-Accel",     PALETTE['purple'],  "{:.2f} m/s²"),
            ('temp',      "Temp",        PALETTE['red'],     "{:.1f} °C"),
            ('pressure',  "Pressure",    PALETTE['blue'],    "{:.0f} Pa"),
        ]
        for key, label, color, fmt in defs:
            box, lbl = stat_card(label, color)
            self._stat[key] = (lbl, fmt)
            card_row.addWidget(box)
        v.addLayout(card_row)


        # Altitude plot (baro + kalman overlay)
        self.plt_alt = make_plot("Altitude", "m")
        self.crv_baro  = self.plt_alt.plot(pen=pg.mkPen(PALETTE['cyan'],   width=2), name="Barometer")
        self.crv_kalt  = self.plt_alt.plot(pen=pg.mkPen(PALETTE['yellow'], width=2, style=Qt.PenStyle.DashLine), name="Kalman")


        # Velocity + Accel side by side
        self.plt_vel = make_plot("Kalman Velocity", "m/s")
        self.crv_vel = self.plt_vel.plot(pen=pg.mkPen(PALETTE['orange'], width=2), name="Velocity")


        self.plt_acc = make_plot("Kalman Acceleration", "m/s²")
        self.crv_kacc = self.plt_acc.plot(pen=pg.mkPen(PALETTE['purple'], width=2), name="Accel")


        v.addWidget(self.plt_alt, 3)
        bot = QHBoxLayout()
        bot.addWidget(self.plt_vel)
        bot.addWidget(self.plt_acc)
        v.addLayout(bot, 2)
        return w


    # ── IMU tab ───────────────────────────────────────────────────────────────


    def _tab_imu(self) -> QWidget:
        w = QWidget()
        v = QVBoxLayout(w)


        # Accelerometer
        self.plt_accel = make_plot("Accelerometer (total)", "m/s²")
        self.crv_ax = self.plt_accel.plot(pen=pg.mkPen(PALETTE['red'],  width=2), name="X")
        self.crv_ay = self.plt_accel.plot(pen=pg.mkPen(PALETTE['lime'], width=2), name="Y")
        self.crv_az = self.plt_accel.plot(pen=pg.mkPen(PALETTE['blue'], width=2), name="Z")


        # Linear accel
        self.plt_lin = make_plot("Linear Acceleration", "m/s²")
        self.crv_lx = self.plt_lin.plot(pen=pg.mkPen(PALETTE['red'],  width=2), name="X")
        self.crv_ly = self.plt_lin.plot(pen=pg.mkPen(PALETTE['lime'], width=2), name="Y")
        self.crv_lz = self.plt_lin.plot(pen=pg.mkPen(PALETTE['blue'], width=2), name="Z")


        # Gyro
        self.plt_gyro = make_plot("Gyroscope", "rad/s")
        self.crv_wx = self.plt_gyro.plot(pen=pg.mkPen(PALETTE['red'],    width=2), name="X")
        self.crv_wy = self.plt_gyro.plot(pen=pg.mkPen(PALETTE['lime'],   width=2), name="Y")
        self.crv_wz = self.plt_gyro.plot(pen=pg.mkPen(PALETTE['orange'], width=2), name="Z")


        row1 = QHBoxLayout()
        row1.addWidget(self.plt_accel)
        row1.addWidget(self.plt_lin)
        v.addLayout(row1, 1)
        v.addWidget(self.plt_gyro, 1)
        return w


    # ── GPS tab ───────────────────────────────────────────────────────────────


    def _tab_gps(self) -> QWidget:
        w = QWidget()
        v = QVBoxLayout(w)


        top = QHBoxLayout()
        lbl = QLabel("Live NMEA stream")
        lbl.setStyleSheet(f"color:{PALETTE['cyan']}; font-weight:bold; font-size:13px;")
        top.addWidget(lbl)
        top.addStretch()
        clr_btn = QPushButton("Clear")
        clr_btn.setFixedWidth(55)
        clr_btn.clicked.connect(lambda: self.gps_text.clear())
        top.addWidget(clr_btn)
        v.addLayout(top)


        self.gps_text = QTextEdit()
        self.gps_text.setReadOnly(True)
        self.gps_text.setFont(QFont("Courier New", 11))
        v.addWidget(self.gps_text)
        return w


    # ── Terminal ──────────────────────────────────────────────────────────────


    def _build_terminal(self) -> QWidget:
        w = QWidget()
        w.setStyleSheet(
            f"background-color: {PALETTE['panel']};"
            f"border: 1px solid {PALETTE['border']}; border-radius: 5px;"
        )
        v = QVBoxLayout(w)
        v.setContentsMargins(8, 6, 8, 6)
        v.setSpacing(4)


        hdr = QHBoxLayout()
        title = QLabel("⌨  Ground Station Terminal")
        title.setStyleSheet(f"color:{PALETTE['cyan']}; font-weight:bold; font-size:13px;")
        hdr.addWidget(title)
        hdr.addStretch()
        hint = QLabel("Commands:  ping  |  reboot  |  freq <MHz>  |  <raw text>")
        hint.setStyleSheet(f"color:{PALETTE['dim']}; font-size:11px;")
        hdr.addWidget(hint)
        clr = QPushButton("Clear")
        clr.setFixedWidth(50)
        clr.clicked.connect(lambda: self.term_log.clear())
        hdr.addWidget(clr)
        v.addLayout(hdr)


        self.term_log = QTextEdit()
        self.term_log.setReadOnly(True)
        v.addWidget(self.term_log, 1)


        inp_row = QHBoxLayout()
        self.cmd_in = QLineEdit()
        self.cmd_in.setPlaceholderText("Enter command and press Enter…")
        self.cmd_in.setFont(QFont("Courier New", 11))
        self.cmd_in.returnPressed.connect(self._send_cmd)
        inp_row.addWidget(self.cmd_in)
        send_btn = QPushButton("Send ↵")
        send_btn.setFixedWidth(75)
        send_btn.clicked.connect(self._send_cmd)
        inp_row.addWidget(send_btn)
        v.addLayout(inp_row)


        return w


    # ── Slots / Logic ─────────────────────────────────────────────────────────


    def _vdiv(self) -> QFrame:
        f = QFrame()
        f.setFrameShape(QFrame.Shape.VLine)
        f.setStyleSheet(f"color: {PALETTE['border']};")
        f.setFixedWidth(2)
        return f


    def _refresh_ports(self):
        self.port_cb.clear()
        ports = [p.device for p in serial.tools.list_ports.comports()]
        if ports:
            self.port_cb.addItems(ports)
        else:
            self.port_cb.addItem("(no ports found)")


    def _toggle_connect(self):
        if not self._connected:
            port = self.port_cb.currentText()
            baud = int(self.baud_cb.currentText())
            self._worker.connect(port, baud)
        else:
            self._worker.disconnect()


    def _on_conn(self, ok: bool, msg: str):
        self._connected = ok
        if ok:
            self.conn_btn.setText("Disconnect")
            self.conn_btn.setObjectName("btn_disconnect")
            self.status_lbl.setText(f"● {msg}")
            self.status_lbl.setStyleSheet(f"color:{PALETTE['green']}; font-weight:bold;")
        else:
            self.conn_btn.setText("Connect")
            self.conn_btn.setObjectName("btn_connect")
            self.status_lbl.setText("● Disconnected")
            self.status_lbl.setStyleSheet(f"color:{PALETTE['red']}; font-weight:bold;")
        self.conn_btn.style().unpolish(self.conn_btn)
        self.conn_btn.style().polish(self.conn_btn)
        self._term_sys(msg)


    def _on_line(self, line: str):


        # Raw log
        safe = line.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
        self.term_log.append(f"<span style='color:#558855'>{safe}</span>")
        sb = self.term_log.verticalScrollBar()
        sb.setValue(sb.maximum())


        result = parse_line(line)
        if result is None:
            return


        kind, payload = result


        if kind == 'RSSI':
            v = payload
            STORE.rssi = v
            color = PALETTE['green'] if v > -95 else PALETTE['yellow'] if v > -110 else PALETTE['red']
            self.rssi_lbl.setText(f"{v} dBm")
            self.rssi_lbl.setStyleSheet(f"color:{color}; font-weight:bold;")
        elif kind == 'ALT':
            STORE.alt_t.append(payload['t'])
            STORE.alt_altitude.append(payload['altitude'])
            STORE.alt_temp.append(payload['temp'])
            STORE.alt_pressure.append(payload['pressure'])
            STORE.latest.update({
                'baro_alt': payload['altitude'],
                'temp':     payload['temp'],
                'pressure': payload['pressure'],
            })
            STORE.packet_count += 1
            self.setText(str(STORE.packet_count))
        elif kind == 'KALMAN':
            STORE.kal_t.append(payload['t'])
            STORE.kal_alt.append(payload['alt'])
            STORE.kal_vel.append(payload['vel'])
            STORE.kal_acc.append(payload['acc'])
            self.state_lbl.setText(str(payload['state']))
            STORE.latest.update({
                'state': payload['state'],
                'kal_alt': payload['alt'],
                'kal_vel': payload['vel'],
                'kal_acc': payload['acc'],
            })
            STORE.packet_count += 1
            self.pkt_lbl.setText(str(STORE.packet_count))

        elif kind == 'IMU':
            t = payload['t']
            STORE.imu_t.append(t)
            STORE.imu_ax.append(payload['ax']); STORE.imu_ay.append(payload['ay']); STORE.imu_az.append(payload['az'])
            STORE.imu_lx.append(payload['lx']); STORE.imu_ly.append(payload['ly']); STORE.imu_lz.append(payload['lz'])
            STORE.imu_gx.append(payload['gx']); STORE.imu_gy.append(payload['gy']); STORE.imu_gz.append(payload['gz'])
            STORE.imu_wx.append(payload['wx']); STORE.imu_wy.append(payload['wy']); STORE.imu_wz.append(payload['wz'])

            STORE.packet_count += 1
            self.pkt_lbl.setText(str(STORE.packet_count))
        elif kind == 'GPS':
            STORE.gps_history.append(payload)
            self.gps_text.append(
                f"<span style='color:#668866'>[{payload['ts']}]</span> "
                f"<span style='color:#88ffaa'>{payload['nmea']}</span>"
            )
            gsb = self.gps_text.verticalScrollBar()
            gsb.setValue(gsb.maximum())
            STORE.packet_count += 1
            self.pkt_lbl.setText(str(STORE.packet_count))


    def _refresh_plots(self):
        # Altitude tab
        if STORE.alt_t:
            t = list(STORE.alt_t)
            self.crv_baro.setData(t, list(STORE.alt_altitude))
        if STORE.kal_t:
            t = list(STORE.kal_t)
            self.crv_kalt.setData(t, list(STORE.kal_alt))
            self.crv_vel.setData(t, list(STORE.kal_vel))
            self.crv_kacc.setData(t, list(STORE.kal_acc))


        # IMU tab
        if STORE.imu_t:
            t = list(STORE.imu_t)
            self.crv_ax.setData(t, list(STORE.imu_ax))
            self.crv_ay.setData(t, list(STORE.imu_ay))
            self.crv_az.setData(t, list(STORE.imu_az))
            self.crv_lx.setData(t, list(STORE.imu_lx))
            self.crv_ly.setData(t, list(STORE.imu_ly))
            self.crv_lz.setData(t, list(STORE.imu_lz))
            self.crv_wx.setData(t, list(STORE.imu_wx))
            self.crv_wy.setData(t, list(STORE.imu_wy))
            self.crv_wz.setData(t, list(STORE.imu_wz))


        # Stat cards
        for key, (lbl, fmt) in self._stat.items():
            if key in STORE.latest:
                lbl.setText(fmt.format(STORE.latest[key]))


    def _send_cmd(self):
        cmd = self.cmd_in.text().strip()
        if not cmd:
            return
        if not self._connected:
            self._term_err("Not connected")
            return
        self.term_log.append(
            f"<span style='color:{PALETTE['cyan']}'>[TX] {cmd}</span>"
        )
        self._worker.send(cmd)
        self.cmd_in.clear()


    def _term_sys(self, msg: str):
        self.term_log.append(
            f"<span style='color:{PALETTE['dim']}'>[sys] {msg}</span>"
        )


    def _term_err(self, msg: str):
        self.term_log.append(
            f"<span style='color:{PALETTE['red']}'>[ERR] {msg}</span>"
        )




# ──────────────────────────────────────────────────────────────────────────────
# Entry point
# ──────────────────────────────────────────────────────────────────────────────
def main():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    win = SOARGroundStation()
    win.show()
    sys.exit(app.exec())




if __name__ == '__main__':
    main()
