//ARCGIS FRONT END CODE BEGINS --------------------------------------------------------------------------------------------------
/*
    Use this to first create a feature layer: Mapping APIs and Location Services: https://developers.arcgis.com/documentation/mapping-apis-and-services/data-hosting/tutorials/tools/define-a-new-feature-layer/
    Then use this to edit the feature layer and draw/modify a web map but only through GUI: ArcGIS Maps SDK for JS https://developers.arcgis.com/javascript/latest/tutorials/edit-feature-data/#prerequisites
    But then use this to add REST API usage so the code can do this without GUI: ArcGIS REST JS https://developers.arcgis.com/arcgis-rest-js/layers/edit-feature-data/ 
    NOTE: This is a light description on tooling: ArcGIS Maps SDK for JS https://developers.arcgis.com/javascript/latest/tooling-intro/
*/
require([
	"esri/config",
	"esri/WebMap",
	"esri/views/MapView",
	"esri/widgets/ScaleBar",
	"esri/widgets/Legend",

	"esri/layers/FeatureLayer",
	"esri/widgets/Editor",
], function (esriConfig, WebMap, MapView, ScaleBar, Legend, FeatureLayer, Editor) {
	// Feature Layer Things:
	// Reference a feature layer to edit
	const SOAR_GPS_DATA_FEATURE_LAYER = new FeatureLayer({
		url: "https://services1.arcgis.com/sQOueNHZBxRJjRN3/arcgis/rest/services/SOAR_GPS_Data_1/FeatureServer",
	});

	esriConfig.apiKey = "AAPK79842bdf715e40379281a8d783198227FFYy_xmKDj0ys_-3KxRmMiFhtsF2f8HH8TMIzVfoN26e_dAcRRgmYsnivKJMf20w";

	const webmap = new WebMap({
		portalItem: {
			id: "812c6c7bf59542d9a957220725962832",
		},
		layers: [SOAR_GPS_DATA_FEATURE_LAYER],
	});

	const view = new MapView({
		container: "viewDiv",
		map: webmap,
	});

	// Makes the map scale bar
	const scalebar = new ScaleBar({
		view: view,
	});
	view.ui.add(scalebar, "bottom-left");

	// Makes the Legend
	const legend = new Legend({
		view: view,
	});
	view.ui.add(legend, "top-right");

	// Editor Widget
	const editor = new Editor({
		view: view,
	});
	// Add widget to the view
	view.ui.add(editor, "top-right");
});

// ------------------------------------------------------------------------------------------------------------------------------
//FRONT END CODE BEGINS ---------------------------------------------------------------------------------------------------------

// Important Global Variables
let active_map = "";
let theme_value = "Light";
let payload = false;

let varn_map = document.getElementById("varn_map");
let usf_map = document.getElementById("usf_map");
let palm_bay_map = document.getElementById("palm_bay_map");
let huntsville_map = document.getElementById("huntsville_map");

window.addEventListener("load", () => {
	if (active_map == "USF") {
		draw_usf();
	}
	if (active_map == "Varn") {
		draw_varn();
	}
	//Add logic for Palm Bay and Huntsville when you make those maps
});

varn_map.addEventListener("click", () => {
	draw_varn();
	active_map = "Varn";
});
usf_map.addEventListener("click", () => {
	draw_usf();
	active_map = "USF";
});

// Sets Payload Global Variable
document.getElementById("rocket_identifier").addEventListener("click", () => {
	payload = false;
});
document.getElementById("payload_identifier").addEventListener("click", () => {
	payload = true;
});

//Processes and plots the data on the map
function plot_varn(section, nmea_sentence) {
	// Sample String:
	// USF
	// <LORA>+RCV=7,31,GPS: A,2803.9643,N,08225.0154,W\r</LORA>
	// Varn (modified and updated)
	// <LORA>+RCV=7,31,GPS: A,2805.5864,N,08210.5439,W\r</LORA>
	// Old Sample
	// $GNRMC,025859.000,A,2805.5864,N,08210.5439,W,0.10,174.77,181123,,,A*68

	//Grab the important information from the NMEA sentence
	/*WARNING--------------------------------------------------
        WHEN USING THE INPUT FIELD TO ENTER IN SAMPLE NMEA SENTENCE FORMATS
        MAKE SURE TO INCLUDE THE SPACE BEFORE THE SAMPLE
        So for example, if you had the following sentence that I will surround in quotes
        "A,2803.9643,N,08225.0154,W\r"
        Then when entering it into the field, you MUST enter it like this:
        " A,2803.9643,N,08225.0154,W\r"
        This will have a space at the front of whatever you enter
        Another example, let us say that you wished to enter the word "Hi"
        then you would enter " Hi"
    */
	// A,2805.5864,N,08210.5439,W\r
	console.log(`Look at me: ${nmea_sentence}`);
	let status = nmea_sentence.slice(1, 2);
	if (section == false) {
		payload = false;
	}
	if (section == true) {
		payload = true;
	}
	if (status == "V") {
		if (payload == false) {
			console.error(`Invalid GPS String: ${status}\nTransmitter: Rocket System`);
			logger(`Invalid GPS String: ${status}\nTransmitter: Rocket System`, true);
		} else {
			if (payload == true) {
				console.error(`Invalid GPS String: ${status}\nTransmitter: Payload System`);
				logger(`Invalid GPS String: ${status}\nTransmitter: Payload System`, true);
			}
		}
		return;
	}
	if (status == "A") {
		if (payload == false) {
			console.log(`Valid GPS String: ${status}\nTransmitter: Rocket System`);
			logger(`Valid GPS String: ${status}\nTransmitter: Rocket System`);
		} else {
			if (payload == true) {
				console.log(`Valid GPS String: ${status}\nTransmitter: Payload System`);
				logger(`Valid GPS String: ${status}\nTransmitter: Payload System`);
			}
		}
	}

	let minuteslat = nmea_sentence.slice(5, 12);
	let minuteslong = nmea_sentence.slice(18, 25);
	let total_lat = nmea_sentence.slice(3, 14);
	let total_long = nmea_sentence.slice(15, 27);

	//Latitude-------------------------------------------------------------------------
	/*Latitude Formula explanation
    1 min = 1 nm = 6076.12ft
    X min = Y ft
    1/(1/X) = X
    6076.12/(1/X) = Y
    so 6076.12 * X = Y
    This will give us Y, the distance in feet.

    We need to convert this to pixels.
    To do this, we use the previously found calculations that determined that
    1 px = 300/41 ft
    Thus,
    Y / (300/41) = Z pixels
    */
	// lat = (lat - 2805.5864) * 300;
	//Note: In the formula you need to separate minutes from degrees. "minuteslat" already is pure minutes
	//But when adding in a secondary input source, you need to remove the degrees and then subtract from the origin
	// let real_lat = (6076.12 * (minuteslat - 5.5864)) / (300/41);
	let real_lat = (6076.12 * (minuteslat - 5.493)) / (300 / 41);

	//Longitude------------------------------------------------------------------------
	/*Longitude Formula Explanation
    The distance between degrees of latitude remains consistent, but due to the way that longitudal lines converge as they
    get closer to the poles, the distances between lines of longitude vary. This can be accounted for by the following formula:

    1 min = cos(latitude) nm = cos(latitude)

    By this logic, at 0 degrees latitude, the distance between longitudinal lines will be 1 nm for every degree. This will
    complement the fact that at in latitude, every minute is 1 nm.

    1 min = cos(latitude) nm
    First, convert nm to ft
    1 nm = 6076.12 ft
    A nm = B ft
    6076.12 * A = Y ft
    So 1 min = 6076.12 * cos(latitude)

    X min = Y ft
    6076.12 * cos(latitude) * X min = Y ft

    We need to convert this to pixels.
    To do this, we use the previously found calculations that determined that
    1 px = 300/41 ft
    Thus,
    Y / (300/41) = Z pixels
    */
	// long = (long - 8210.5440) * 300;
	// let real_long = (6076.12 * (Math.cos(real_lat)) * (minuteslong - 10.5439)) / (300/41);
	let real_long = (6076.12 * Math.cos(real_lat) * (minuteslong - 10.249)) / (300 / 41);

	//-------------------------------------------------------------------------------------------------------------------------

	/*Here is a sample NMEA sentence from Varn Ranch that can be used to test functioning
    $GNRMC,025859.000,A,2805.5864,N,08210.5439,W,0.10,174.77,181123,,,A*68
    NOTE: The GPS gives data in the following format:
    Latitude: DDMM.MMMM (D representing Degrees, and M for minutes)
    Longitude: DDDMM.MMMM (D representing Degrees, and M for minutes)
    Understand that each Degree has 60 Minutes, and each Minute has 60 Seconds*/

	console.log(`Pixels from Origin:\nLat: ${real_lat}\nLong: ${real_long}`);
	logger(`Pixels from Origin:\nLat: ${real_lat}\nLong: ${real_long}`);

	// Optimize the total_lat and total_long variables for display in emergency scenarios
	//The optimal format of output would be in DD°MM'SS'
	let display_degrees_lat = total_lat.slice(0, 2);
	let display_minutes_lat = total_lat.slice(2, 4);
	let old_display_seconds_lat = total_lat.slice(4, 9);
	old_display_seconds_lat = old_display_seconds_lat * 60;
	let display_seconds_lat = `${old_display_seconds_lat}`;
	display_seconds_lat = display_seconds_lat.slice(0, 6);
	Number.parseFloat(display_seconds_lat);
	let display_direction_lat = total_lat.slice(10, 11);

	let display_degrees_long = total_long.slice(0, 3);
	let display_minutes_long = total_long.slice(3, 5);
	let old_display_seconds_long = total_long.slice(5, 10);
	old_display_seconds_long = old_display_seconds_long * 60;
	let display_seconds_long = `${old_display_seconds_long}`;
	display_seconds_long = display_seconds_long.slice(0, 6);
	Number.parseFloat(display_seconds_long);
	let display_direction_long = total_long.slice(11, 12);

	let display_dms_lat = `${display_degrees_lat}°${display_minutes_lat}'${display_seconds_lat}"${display_direction_lat}`;
	let display_dms_long = `${display_degrees_long}°${display_minutes_long}'${display_seconds_long}"${display_direction_long}`;
	console.log(`DMS Format\nLatitude: ${display_dms_lat}\nLongitude: ${display_dms_long}`);
	logger(`DMS Format\nLatitude: ${display_dms_lat}\nLongitude: ${display_dms_long}`);

	//Display on the coordinate the received information:
	let side_display_lat = undefined;
	let side_display_long = undefined;
	if (payload == false) {
		side_display_lat = document.getElementById("side_display_lat_rocket");
		side_display_long = document.getElementById("side_display_long_rocket");
	} else {
		if (payload == true) {
			side_display_lat = document.getElementById("side_display_lat_payload");
			side_display_long = document.getElementById("side_display_long_payload");
		}
	}
	let side_lat_info = document.createTextNode(display_dms_lat);
	side_display_lat.textContent = "";
	side_display_lat.textContent = "Latitude: ";
	side_display_lat.appendChild(side_lat_info);

	let side_long_info = document.createTextNode(display_dms_long);
	side_display_long.textContent = "";
	side_display_long.textContent = "Longitude: ";
	side_display_long.appendChild(side_long_info);
}

function plot_usf(section, nmea_sentence) {
	// console.log(nmea_sentence)
	// USF Base Map Coordinates: 28°03'57.858"N,82°25'0.924"W

	//Grab the important information from the NMEA sentence
	/*WARNING--------------------------------------------------
        WHEN USING THE INPUT FIELD TO ENTER IN SAMPLE NMEA SENTENCE FORMATS
        MAKE SURE TO INCLUDE THE SPACE BEFORE THE SAMPLE
        So for example, if you had the following sentence that I will surround in quotes
        "A,2803.9643,N,08225.0154,W\r"
        Then when entering it into the field, you MUST enter it like this:
        " A,2803.9643,N,08225.0154,W\r"
        This will have a space at the front of whatever you enter
        Another example, let us say that you wished to enter the word "Hi"
        then you would enter " Hi"
    */
	// A,2803.9643,N,08225.0154,W\r
	console.log(`Look at me: ${nmea_sentence}`);
	let status = nmea_sentence.slice(1, 2);
	if (section == false) {
		payload = false;
	}
	if (section == true) {
		payload = true;
	}
	if (status == "V") {
		console.error("Invalid GPS String: " + status);
		logger("Invalid GPS String: " + status, true);
		return;
	}
	if (status == "A") {
		console.log("Valid GPS String: " + status);
		logger("Valid GPS String: " + status);
	}

	let minuteslat = nmea_sentence.slice(5, 12);
	let minuteslong = nmea_sentence.slice(18, 25);
	let total_lat = nmea_sentence.slice(3, 14);
	let total_long = nmea_sentence.slice(15, 27);

	// <LORA>+RCV=7,31,GPS: A,2803.9643,N,08225.0154,W\r</LORA>
	let real_lat = (6076.12 * (minuteslat - 3.9643)) / (300 / 41);
	let real_long = (6076.12 * Math.cos(real_lat) * (minuteslong - 25.0154)) / (300 / 41);
	console.log(`Pixels from Origin:\nLat: ${real_lat}\nLong: ${real_long}`);
	logger(`Pixels from Origin:\nLat: ${real_lat}\nLong: ${real_long}`);

	// Optimize the total_lat and total_long variables for display in emergency scenarios
	//The optimal format of output would be in DD°MM'SS'
	let display_degrees_lat = total_lat.slice(0, 2);
	let display_minutes_lat = total_lat.slice(2, 4);
	let old_display_seconds_lat = total_lat.slice(4, 9);
	old_display_seconds_lat = old_display_seconds_lat * 60;
	let display_seconds_lat = `${old_display_seconds_lat}`;
	display_seconds_lat = display_seconds_lat.slice(0, 6);
	Number.parseFloat(display_seconds_lat);
	let display_direction_lat = total_lat.slice(10, 11);

	let display_degrees_long = total_long.slice(0, 3);
	let display_minutes_long = total_long.slice(3, 5);
	let old_display_seconds_long = total_long.slice(5, 10);
	old_display_seconds_long = old_display_seconds_long * 60;
	let display_seconds_long = `${old_display_seconds_long}`;
	display_seconds_long = display_seconds_long.slice(0, 6);
	Number.parseFloat(display_seconds_long);
	let display_direction_long = total_long.slice(11, 12);

	let display_dms_lat = `${display_degrees_lat}°${display_minutes_lat}'${display_seconds_lat}"${display_direction_lat}`;
	let display_dms_long = `${display_degrees_long}°${display_minutes_long}'${display_seconds_long}"${display_direction_long}`;
	console.log(`Latitude: ${display_dms_lat}\nLongitude: ${display_dms_long}`);
	logger(`Latitude: ${display_dms_lat}\nLongitude: ${display_dms_long}`);

	//Display on the coordinate the received information:
	let side_display_lat = undefined;
	let side_display_long = undefined;
	if (payload == false) {
		side_display_lat = document.getElementById("side_display_lat_rocket");
		side_display_long = document.getElementById("side_display_long_rocket");
	} else {
		if (payload == true) {
			side_display_lat = document.getElementById("side_display_lat_payload");
			side_display_long = document.getElementById("side_display_long_payload");
		}
	}
	let side_lat_info = document.createTextNode(display_dms_lat);
	side_display_lat.textContent = "";
	side_display_lat.textContent = "Latitude: ";
	side_display_lat.appendChild(side_lat_info);

	let side_long_info = document.createTextNode(display_dms_long);
	side_display_long.textContent = "";
	side_display_long.textContent = "Longitude: ";
	side_display_long.appendChild(side_long_info);
}

//Code that plots values from a front-end input NMEA sentence
document.getElementById("dummy_plot").addEventListener("click", () => {
	let nmea = document.getElementById("live_nmea").value;
	console.log("hi");
	// plot(time, lat, long);
	// plot_varn(nmea);
	// plot_usf(nmea);

	if (active_map == "USF") {
		plot_usf(nmea);
	}
	if (active_map == "Varn") {
		plot_varn(nmea);
	}
});

// Code that logs information passed to it to the Console on screen
let logging_object = document.getElementById("live_console");
let logger_reference_node = document.createTextNode("");
logging_object.appendChild(logger_reference_node);
function logger(information, error = false) {
	// Interestingly, the logger HTML element needed a CSS value of white-space: pre-line;
	// https://stackoverflow.com/questions/9980416/how-can-i-insert-new-line-carriage-returns-into-an-element-textcontent
	let new_text = document.createTextNode(information);

	let new_text_holder = document.createElement("div");
	new_text_holder.appendChild(new_text);
	new_text_holder.className = "live_console_messages_light";

	if (document.getElementById("theme_value").textContent == "Light") {
		new_text_holder.className = "live_console_messages_light";
	}

	if (document.getElementById("theme_value").textContent == "Dark") {
		new_text_holder.className = "live_console_messages_dark";
	}

	if (error == true) {
		if (document.getElementById("theme_value").textContent == "Light") {
			new_text_holder.classList.add("live_console_error_messages_light");
		}
		if (document.getElementById("theme_value").textContent == "Dark") {
			new_text_holder.classList.add("live_console_error_messages_dark");
		}
	}

	logging_object.insertBefore(new_text_holder, logger_reference_node);
	logger_reference_node = new_text_holder;
}

//WEBSOCKET CODE
var socket = io.connect("http://" + document.domain + ":" + location.port);

socket.on("gps_update", (thing) => {
	console.log(thing);
	console.log("GPS UPDATE", thing.section, thing.nmea);
	logger("GPS UPDATE", thing.section, thing.nmea);
	// console.log(data.nmea);
	// plot_varn(data.nmea);
	// plot_usf(data.nmea);

	if (active_map == "USF") {
		plot_usf(thing.section, thing.nmea);
	}
	if (active_map == "Varn") {
		plot_varn(thing.section, thing.nmea);
	}
});

socket.on("gps_message", function (data) {
	console.log(data);
	logger(data);
});

socket.on("log_lora", function (data) {
	console.log(data);
	logger(data);
});

//BACK END CODE BEGINS ----------------------------------------------------------------------------------------------------------
//Script to connect with the server
var server = "http://" + document.domain + ":" + location.port;

//Below script to handle GPS Start button press

// Checks if an entered COM port is valid or not
function isValidComPort(comPort) {
	var pattern = /^COM\d+$/;
	return pattern.test(comPort);
}

// Initiates connection to the back end
var generateBtn = document.getElementById("gps_start");
generateBtn.addEventListener("click", get_response);
function get_response() {
	console.log("Beginning GPS");
	let com_port = document.getElementById("com_port").value;
	if (isValidComPort(com_port) == false) {
		console.error(`Invalid COM Port: ${com_port}`);
		logger(`Invalid COM Port: ${com_port}`, true);
		document.getElementById("invalid_com_port_notif").style.display = "block";
		return;
	}
	if (isValidComPort(com_port) == true) {
		logger(`Valid COM Port: ${com_port}`);
		document.getElementById("invalid_com_port_notif").style.display = "none";
	}

	const fetch_promise = fetch(server + "/telemetry_start/" + com_port);
	fetch_promise
		.then((response) => {
			if (!response.ok) {
				throw new Error("HTTP error: ${response.status}");
			}
			return response.text();
		})
		.then((text) => {
			console.log(text);
			logger(text);
		});
}

document.getElementById("close_serial").addEventListener("click", () => {
	const fetch_promise = fetch(server + "/close_serial");
	fetch_promise
		.then((response) => {
			if (!response.ok) {
				throw new Error(`HTTP error: ${response.status}`);
			}
			return response.text();
		})
		.then((text) => {
			console.log(text);
			logger(text);
		});
});

function SendCommand(command, id = 0) {
	var full_command = `${command},${id}`;
	const fetch_promise = fetch(server + "/command/" + full_command);
	fetch_promise
		.then((response) => {
			if (!response.ok) {
				throw new Error(`HTTP error: ${response.status}`);
				logger(`HTTP error: ${response.status}`);
			}
			return response.text();
		})
		.then((text) => {
			console.log(text);
			logger(text);
		})
		.catch((err) => {
			logger(err, true);
			console.log(err);
		});
}
document.getElementById("live_console_send_msg_btn").addEventListener("click", () => {
	let message = document.getElementById("live_console_send_msg_field").value;
	let id = document.getElementById("live_console_send_msg_id").value;
	SendCommand(message, id);
});

//Sends a random message to the transmitter
var random_cmd = document.getElementById("random_cmd");
random_cmd.addEventListener("click", send_random);
function send_random() {
	const fetch_promise = fetch(server + "/random_cmd");
	fetch_promise
		.then((response) => {
			if (!response.ok) {
				throw new Error("HTTP error: ${response.status}");
			}
			return response.text();
		})
		.then((text) => {
			console.log(text);
		});
}
