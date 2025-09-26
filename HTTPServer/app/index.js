const express = require('express');
const fs = require('fs');
const path = require('path');
const config = process.env.hasOwnProperty('CONFIG') ? JSON.parse(process.env.CONFIG) : require('./config.js');
const StateController = require('./stateController');
const processVideos = require('./processVideos.js');

const app = express();
const PORT = process.env.PORT || 8080;

processVideos();//invoke immediately
setInterval(processVideos, 5 * 60 * 1000);  // Invoke again every 5 minutes

app.get('/image', (req, res) => {

  if (!req.query.displayId) {
    res.status(400).send('Missing displayId query parameter');
    return;
  }

  if (!StateController.hasDisplayWithId(req.query.displayId)) {
    res.status(400).send('No display found for Id = ' + req.query.displayId);
    return;
  }

  if (req.query.batteryVoltage) {
    const voltage = req.query.batteryVoltage;
    console.log(`Battery Voltage: ${voltage}`);

     // Determine the path to the voltages.csv file for the specific display
     const displayDir = path.join(config.DATA_DIR, `display${req.query.displayId}`);
     const csvFilePath = path.join(displayDir, 'voltages.csv');


     // Use the TZ environment variable for the timezone, default to 'America/Chicago'
    const timeZone = process.env.TZ || 'America/Chicago';
    const timestamp = new Date().toLocaleString('en-US', { timeZone });
     const csvLine = `${timestamp},${voltage}\n`;

    // Check if the file exists, if not create it with headers
    if (!fs.existsSync(csvFilePath)) {
      fs.writeFileSync(csvFilePath, 'date,time,voltage\n', 'utf8');
    }

    // Append the new data
    fs.appendFileSync(csvFilePath, csvLine, 'utf8');
  }

  try {
    let bitmapData = StateController.getNextFrame(req.query.displayId);
    res.setHeader('Content-Type', 'image/bmp');
    res.end(bitmapData);
  } catch (err) {
    res.status(500).send('Error reading BMP file: ' + err);
    console.error(err);
  }

});

app.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}/`);
});