#pragma once

char mWebServerHtmlRoot[] PROGMEM = R"=====(
﻿<!DOCTYPE html PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>
<html>

<head>
  <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>
  <meta http-equiv='Content-Style-Type' content='text/css'>
  <title>ESP8266 Test GPIO</title>
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
      mWebSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
      mWebSocket.onopen = onWebSocketOpen;
      mWebSocket.onclose = onWebSocketClose;
      mWebSocket.onerror = onWebSocketError;
      mWebSocket.onmessage = onWebSocketMessage;
    }

    function onWebSocketOpen(aEvent) {
      mWebSocketStatus.innerHTML = 'connected';
      if (mWebSocketReConnectTimerID) {
        window.clearInterval(mWebSocketTimerID);
        mWebSocketReConnectTimerID = 0;
      }
    }

    function onWebSocketClose(aEvent) {
      mWebSocketStatus.innerHTML = 'disconnected';
      if (!mWebSocketReConnectTimerID) {
        mWebSocketReConnectTimerID = window.setInterval(initWebSocket, 5000);
      }
    }

    function onWebSocketError(aEvent) {
      mWebSocketStatus.innerHTML = aEvent;
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
      mTemperatureRef.onchange = function() {
        mWebSocket.send('SetTemperatureRef(' + mTemperatureRef.value + ')');
      };
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
    }
    
    function UpdateSensorStatus(aSensorStatus) {
      mSensorStatus.innerHTML = aSensorStatus;
    }

    function UpdateTemperature(aTemperature) {
      mTemperature.innerHTML = aTemperature + '°C';
    }

    function UpdateHumidity(aHumidity) {
      mHumidity.innerHTML = aHumidity + '%'
    }

    function UpdateTemperatureRef(aTemperatureRef) {
      mTemperatureRef.value = aTemperatureRef;
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

    function UpdateSSR(aSSR) {
      mSSR.bgColor = aSSR > 0.5 ? 'Gold' : 'LightGray';
      mSSR.innerHTML = 100 * aSSR + '%';
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
      <td>Sensor Status</td>
      <td id='mSensorStatus'>(na)</td>
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
      <td>Temperature Ref</td>
      <td><input id='mTemperatureRef' type='number' min='0' max='30' step='0.1' value='(na)'> °C</td>
    </tr>

    <tr>
      <td valign='top'>SSR Control</td>
      <td>
        <input id='mControlModeOff' type='radio' name='control'><label for='mControlModeOff'> Off<label><br>
        <input id='mControlModeOnBelow' type='radio' name='control'><label for='mControlModeOnBelow'> On, if below ref (heater)</label><br>
        <input id='mControlModeOnAbove' type='radio' name='control'><label for='mControlModeOnAbove'> On, if above ref (cooler)</label><br>
        <input id='mControlModeOn' type='radio' name='control'><label for='mControlModeOn'> On</label><br>
      </td>
    </tr>

    <tr>
      <td>SSR</td>
      <td id='mSSR'>(na)</td>
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

