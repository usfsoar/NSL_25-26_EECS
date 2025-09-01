const path = require("path");

module.exports = {
	entry: "/src/npm_functions.js",
	output: {
		filename: "bundle.js",
		path: path.resolve(__dirname, "static"),
	},
	mode: "development",
};
