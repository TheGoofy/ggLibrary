#pragma once

char mWebServerHtmlRoot[] PROGMEM = R"=====(
﻿<!DOCTYPE html PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>
<html>

<head>
  <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>
  <meta http-equiv='Content-Style-Type' content='text/css'>
  <meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>
  <title>ESP8266 Thermostate</title>
  <style type='text/css'>
  
    .selectnone {
      -webkit-user-select: none;
      -khtml-user-select: none;
      -moz-user-select: none;
      -ms-user-select: none;
      user-select: none;
    }
    
  </style>  
  <script language='javascript' type='text/javascript'>

    function initAll() {
      initWebSocket();
      initHtmlElements();
    }
  
    var mWebSocket;
    var mWebSocketReConnectTimerID = 0;

    function initWebSocket() {
      // init web socket client
      mWebSocket = new WebSocket('ws://' + window.location.hostname + ':81/', ['arduino']);
      mWebSocket.onopen = onWebSocketOpen;
      mWebSocket.onclose = onWebSocketClose;
      mWebSocket.onerror = onWebSocketError;
      mWebSocket.onmessage = onWebSocketMessage;
    }

    function onWebSocketOpen(aEvent) {
      console.log(aEvent.data);
      mWebSocketStatus.innerHTML = 'connected';
      if (mWebSocketReConnectTimerID) {
        window.clearInterval(mWebSocketReConnectTimerID);
        mWebSocketReConnectTimerID = 0;
      }
    }

    function onWebSocketClose(aEvent) {
      console.log(aEvent.data);
      mWebSocketStatus.innerHTML = 'disconnected';
      if (!mWebSocketReConnectTimerID) {
        mWebSocketReConnectTimerID = window.setInterval(initWebSocket, 5000);
      }
    }

    function onWebSocketError(aEvent) {
      mWebSocketStatus.innerHTML = aEvent.data;
    }

    function onWebSocketMessage(aEvent) {
      // server sends actual function names, which can directly be executed by the client
      eval(aEvent.data);
    }

    var tControlMode = {
      eOff : 0,
      eOnBelow : 1,
      eOnAbove : 2,
      eOn : 3
    };
  
    function initHtmlElements() {
      mName.onchange = function() {
        mWebSocket.send('SetName(' + mName.value + ')');
      }
      mControlModeOff.onchange = function() {
        mWebSocket.send('SetControlMode(' + tControlMode.eOff + ')');
      }
      mControlModeOnBelow.onchange = function() {
        mWebSocket.send('SetControlMode(' + tControlMode.eOnBelow + ')');
      }
      mControlModeOnAbove.onchange = function() {
        mWebSocket.send('SetControlMode(' + tControlMode.eOnAbove + ')');
      }
      mControlModeOn.onchange = function() {
        mWebSocket.send('SetControlMode(' + tControlMode.eOn + ')');
      }
      mTemperatureRef.onchange = function() {
        mWebSocket.send('SetTemperatureRef(' + mTemperatureRef.value + ')');
      }
      mHysteresis.onchange = function() {
        mWebSocket.send('SetHysteresis(' + mHysteresis.value + ')');
      }
      mOutputAnalog.onchange = function() {
        mWebSocket.send('SetOutputAnalog(' + mOutputAnalog.checked + ')');
      }
      mOutputRange.onchange = function() {
        mWebSocket.send('SetOutput(' + mOutputRange.value + ')');
      }
    }
    
    function UpdateName(aName) {
      mName.value = aName;
    }

    function UpdateSensorStatus(aSensorStatus) {
      mSensorStatus.innerHTML = aSensorStatus;
    }

    function UpdatePressure(aPressure) {
      mPressure.innerHTML = aPressure + ' hPa';
    }

    function UpdateTemperature(aTemperature) {
      mTemperature.innerHTML = aTemperature + ' °C';
    }

    function UpdateHumidity(aHumidity) {
      mHumidity.innerHTML = aHumidity + ' %'
    }

    function UpdateControlMode(aControlMode) {
      switch (aControlMode) {
        case tControlMode.eOff: mControlModeOff.checked = true; break;
        case tControlMode.eOnBelow: mControlModeOnBelow.checked = true; break;
        case tControlMode.eOnAbove: mControlModeOnAbove.checked = true; break;
        case tControlMode.eOn: mControlModeOn.checked = true; break;
        default: alert("unknown control mode"); break;
      }
    }

    function UpdateTemperatureRef(aTemperatureRef) {
      mTemperatureRef.value = aTemperatureRef;
    }

    function UpdateHysteresis(aHysteresis) {
      mHysteresis.value = aHysteresis;
    }

    function UpdateOutputAnalog(aOutputAnalog) {
      mOutputAnalog.checked = aOutputAnalog;
    }

    function UpdateOutput(aOutput) {
      mOutput.bgColor = aOutput > 0.5 ? 'Gold' : 'LightGray';
      mOutputValue.innerHTML = Math.round(100.0 * aOutput) + '%';
      mOutputRange.value = aOutput;
    }

    function UpdateStatusLED(aStatusLED) {
      mStatusLED.bgColor = aStatusLED ? 'LawnGreen' : 'LightGray';
      mStatusLED.innerHTML = aStatusLED;
    }

    function UpdateKey(aKey) {
      mKey.bgColor = aKey ? 'DeepSkyBlue' : 'LightGray';
      mKey.innerHTML = aKey;
    }

  </script>
</head>

<body onload='javascript:initAll()'>
  <table border='0' cellspacing='2' cellpadding='5'>
    
    <tr>
      <td>Web Socket Status</td>
      <td id='mWebSocketStatus'>(na)</td>
    </tr>

    <tr>
      <td>SSR Name</td>
      <td><input id='mName' type='text' value='(na)' style='width:100%' class='r' maxlength='30'></td>
    </tr>

    <tr>
      <td>Sensor Status</td>
      <td id='mSensorStatus'>(na)</td>
    </tr>

    <tr>
      <td>Pressure</td>
      <td id='mPressure'>(na)</td>
    </tr>

    <tr>
      <td>Temperature</td>
      <td id='mTemperature'>(na)</td>
    </tr>

    <tr>
      <td>Humidity</td>
      <td id='mHumidity'>(na)</td>
    </tr>

    <tr>
      <td valign='top'>Output Control</td>
      <td>
        <input id='mControlModeOff' type='radio' name='control'><label for='mControlModeOff'> Off<label><br>
        <input id='mControlModeOnBelow' type='radio' name='control'><label for='mControlModeOnBelow'> On, if below ref (heater)</label><br>
        <input id='mControlModeOnAbove' type='radio' name='control'><label for='mControlModeOnAbove'> On, if above ref (cooler)</label><br>
        <input id='mControlModeOn' type='radio' name='control'><label for='mControlModeOn'> On</label><br>
      </td>
    </tr>

    <tr>
      <td>Temperature Ref</td>
      <td><input id='mTemperatureRef' type='number' min='-100' max='100' step='0.1' value='0'> °C</td>
    </tr>

    <tr>
      <td>Hysteresis</td>
      <td><input id='mHysteresis' type='number' min='0' max='100' step='0.1' value='0'> °C</td>
    </tr>

    <tr>
      <td>Analog Out</td>
      <td><input id='mOutputAnalog' type='checkbox'></td>
    </tr>

    <tr>
      <td>Output</td>
      <td id='mOutput'><table cellpadding='0' cellspacing='0' border='0'><tr>
        <td><input id='mOutputRange' type='range' name='outputrange' min='0' max='1' step='0.1' value='0'></td>
        <td>&nbsp;<span id='mOutputValue'>(na)</span></td>
      </tr></table></td>
    </tr>

    <tr>
      <td>Status LED</td>
      <td id='mStatusLED'>(na)</td>
    </tr>

    <tr>
      <td>Key</td>
      <td id='mKey'>(na)</td>
    </tr>

  </table>
</body>

</html>
)=====";
