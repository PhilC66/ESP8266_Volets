const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang='fr'><head><meta http-equiv='refresh'  name='viewport' content='width=device-width, initial-scale=1 user-scalable=no'/>
<style>
  body {
    font-family: Arial, Helvetica, sans-serif;
    padding: 1%;
  }
  .button1 {
  background-color: #4caf50;
  border: none;
  border-radius: 8px;
  color: white;
  padding: 15px 15px;
  text-align: center;
  font-size: 14px;
  cursor: pointer;
  }
  .button1:hover {
    background-color: green;
  }
  .button2 {
    background-color: #008cba; /* Blue */
    border: none;
    border-radius: 8px;
    color: white;
    padding: 15px 15px;
    text-align: center;
    font-size: 14px;
    cursor: pointer;
  }

  .button2:hover {
    background-color: #00739c;
  }
</style>
<title>Volets</title></head>
<body>
<table>
  <thead>
    <tr>
      <th colspan="3"><h1>Volets Roulants</h1></th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td align="center">Etage</td>
      <td align="center"><form action='/' method='POST'><button type='button submit' name='D0' value='M' class='button1'>Ouvrir</button></form></td>
      <td align="center"><form action='/' method='POST'><button type='button submit' name='D1' value='D' class='button2'>Fermer</button></form></td>
    </tr>
    <tr>
      <td align="center">Rdc</td>
      <td align="center"><form action='/' method='POST'><button type='button submit' name='D2' value='M' class='button1'>Ouvrir</button></form></td>
      <td align="center"><form action='/' method='POST'><button type='button submit' name='D3' value='D' class='button2'>Fermer</button></form></td>
    </tr>
  </tbody>
</table>
</body></html>
)=====";