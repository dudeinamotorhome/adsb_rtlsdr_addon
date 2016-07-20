var adsb = require("../build/Release/adsb_rtlsdr_addon");

var radios = adsb.getRadioList();
console.log('getRadioList() returns:\n' + JSON.stringify(radios));

var b = adsb.initRadioByID(0, false);
console.log('initRadioByID returned: ' + b);

var gainSettings = adsb.getGainSettings();
console.log('Gain Settings:\n' + JSON.stringify(gainSettings));

var freqSettings = adsb.getFreqSettings();
console.log('Freq Settings:\n' + JSON.stringify(freqSettings));

//adsb.closeRadio();
