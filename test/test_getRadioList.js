var adsb = require("../build/Release/adsb_rtlsdr_addon");

var radios = adsb.getRadioList();

console.log('getRadioList() returns:\n' + JSON.stringify(radios));
