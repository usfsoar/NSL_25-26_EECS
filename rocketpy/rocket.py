from rocketpy import Environment, SolidMotor, Rocket, Flight
import datetime
import math

tomorrow = datetime.date.today() + datetime.timedelta(days=1)

env = Environment(latitude=28.0, longitude=-82.0, elevation=10)
env.set_date(
    (tomorrow.year, tomorrow.month, tomorrow.day, 12)
)  # Hour given in UTC time
env.set_atmospheric_model(type="Forecast", file="GFS")

motor_total_kg = 8.99       # kg (total motor weight you provided)
prop_mass_kg = 5.66         # kg (propellant mass you provided)
dry_mass = motor_total_kg - prop_mass_kg

nozzle_radius = 0.049        # m  (98 mm / 2)
motor_length = 0.732         # m  (732 mm)
avg_thrust = 1661          # N
max_thrust = 2230          # N
total_impulse = 10369      # N*s
burn_time = 6.23              # s

# Approximate dry inertia (hollow-cylinder/casing approximation)
# Izz ≈ m * r^2
# Ixx = Iyy ≈ 0.5*m*r^2 + (1/12)*m*h^2

Izz = dry_mass * nozzle_radius**2
Ixx = 0.5 * dry_mass * nozzle_radius**2 + (1.0/12.0) * dry_mass * motor_length**2
Iyy = Ixx
dry_inertia = (Ixx, Iyy, Izz)

rho_prop = 1800.0            # kg/m^3 (estimate for composite propellant)
prop_volume = prop_mass_kg / rho_prop
grain_outer_radius = nozzle_radius
grain_initial_inner_radius = 0.0      # assume end-burning (no central port). Set >0 if ported.
grain_initial_height = prop_volume / (math.pi * grain_outer_radius**2)
grain_number = 1
throat_radius = 0.0093  # meters (estimate; replace with real throat radius if available)

M1939W = SolidMotor(
    thrust_source="AeroTech_M1939W.eng",
    dry_mass=dry_mass,
    dry_inertia=dry_inertia,
    nozzle_radius=nozzle_radius,
    grain_number=grain_number,
    grain_density=rho_prop,
    grain_outer_radius=grain_outer_radius,
    grain_initial_inner_radius=grain_initial_inner_radius,
    grain_initial_height=grain_initial_height,
    grain_separation=0.005,
    grains_center_of_mass_position=-motor_length / 2.0,
    center_of_dry_mass_position=-motor_length / 2.0,
    nozzle_position=0.0,
    burn_time=burn_time,
    throat_radius=throat_radius,
    coordinate_system_orientation="combustion_chamber_to_nozzle"
)

loaded_mass_with_motor = 23.5  # kg (value from your OpenRocket PDF earlier)
motor_mass_used = motor_total_kg
rocket_mass_without_motor = loaded_mass_with_motor - motor_mass_used  # = 14.512 kg

body_diameter = 0.157  # m (from your OR file earlier)
rocket_radius = body_diameter / 2.0

rocket_inertia = (Ixx,Iyy,Izz)

rocket = Rocket(
    radius=rocket_radius,
    mass=rocket_mass_without_motor,
    inertia=rocket_inertia,
    power_off_drag="Cd_vs_Mach_openrocket_clean.csv",
    power_on_drag="Cd_vs_Mach_openrocket_clean.csv",
    center_of_mass_without_motor=1.638,
    coordinate_system_orientation="nose_to_tail"
)

rocket.add_motor(M1939W, position=2.794)

# Nose and fins placeholders
nose_cone=rocket.add_nose(
    length=0.762, kind="lvhaack", position=0.0
)

fin_set=rocket.add_trapezoidal_fins(
    n=4,
    root_chord=0.304,
    tip_chord=0.05,
    span=0.157,
    position=2.49,
    cant_angle=0
)

main = rocket.add_parachute(
    name="main",
    cd_s=9.77,
    trigger=183,      # ejection altitude in meters
    sampling_rate=105,
    lag=1.5,
    noise=(0, 8.3, 0.5),
)

drogue = rocket.add_parachute(
    name="drogue",
    cd_s=0.293,
    trigger="apogee",  # ejection at apogee
    sampling_rate=105,
    lag=1.5,
    noise=(0, 8.3, 0.5),
)

"""
test_flight = Flight(
    rocket=rocket, environment=env, rail_length=5.2, inclination=85, heading=0
    )
"""
# tail = rocket.add_tail(
#     top_radius=rocket_radius,
#     bottom_radius=rocket_radius,
#     length=0.559,
#     position=0.206
# )

rocket.all_info()
#rocket.plots.static_margin()
#rocket.draw()
#test_flight.all_info()
