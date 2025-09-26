const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');
const config = process.env.hasOwnProperty('CONFIG') ? JSON.parse(process.env.CONFIG) : require('./config.js');

const videoExtensions = ['mkv', 'mp4', 'mov', 'avi', 'flv', 'wmv', 'webm', 'm4v', '3gp', '3g2', 'f4v', 'm2ts', 'mpeg', 'mpg', 'mxf', 'ts', 'vob', 'rm', 'asf', 'amv', 'm2v'];

function scanDirectory(directory) {
    fs.readdirSync(directory).forEach(file => {
        const absolutePath = path.join(directory, file);
        const fileStat = fs.statSync(absolutePath);

        if (fileStat.isDirectory()) {
            if (file.startsWith('display')) {
                scanDirectory(absolutePath);
            }
        } else if (videoExtensions.includes(path.extname(file).slice(1).toLowerCase())) {
            const filename = path.basename(file, path.extname(file));  // Added this line
            const bitmapDir = path.join(directory, `${filename}_bitmap_frames`);
            if (!fs.existsSync(bitmapDir)) {
                console.log("Processing " + absolutePath);
                exec(`./convertVideoToBwBmp.sh "${absolutePath}"`, (error, stdout, stderr) => {
                    if (error) {
                        console.error(`Error executing script: ${error}`);
                        return;
                    }
                    console.log(`stdout: ${stdout}`);
                    console.error(`stderr: ${stderr}`);
                });
                throw new Error('StopIteration');
            }
        }
    });
}

function processVideos() {
    try {
        if (!fs.existsSync(path.join(config.DATA_DIR, 'temp'))) {
            console.log((new Date()).toISOString() + " Looking for videos to process...");
            scanDirectory(config.DATA_DIR);
        } else {
            console.log((new Date()).toISOString() + " A video is being processed.");
        }
    } catch (error) {
        if (error.message !== 'StopIteration') {
            throw error;
        }
    }
}

module.exports = processVideos;