const fs = require('fs');
const path = require('path');
const config = process.env.hasOwnProperty('CONFIG') ? JSON.parse(process.env.CONFIG) : require('./config.js');
const DATA_DIR = config.DATA_DIR;

function hasDisplayWithId(displayId) {
    return fs.existsSync(path.join(DATA_DIR, `display${displayId}`));
}

function getNextFrame(displayId) {

    //determine path to display directory
    const displayDir = path.join(DATA_DIR, `display${displayId}`);
    let stateFilePath = path.join(displayDir, 'state.json');

    //determine state.  Load from file if it exists, otherwise create it
    let state;
    if (fs.existsSync(stateFilePath)) {
        state = JSON.parse(fs.readFileSync(stateFilePath));
        console.log("Loading " + stateFilePath);
    } else {
        const dirs = getSortedVideoDirs(displayDir);
        state = { currentDir: dirs[0], currentFrame: 'frame_0001.bmp' };
        console.log("No existing state.json found in " + displayDir);
    }
    console.log("state is " + JSON.stringify(state));

    let frameDir = path.join(displayDir, state.currentDir);
    const frames = getSortedFrames(frameDir);
    const frameIndex = frames.indexOf(state.currentFrame);

    //determine next frame to show
    if (frameIndex < frames.length - 1) {
        state.currentFrame = frames[frameIndex + 1];
    } else {
        const dirs = getSortedVideoDirs(displayDir);
        const dirIndex = dirs.indexOf(state.currentDir);
        if (dirIndex < dirs.length - 1) {
            state.currentDir = dirs[dirIndex + 1];
            state.currentFrame = 'frame_0001.bmp';
        } else {
            // Already showing last video, reset to first video
            state.currentDir = dirs[0];
            state.currentFrame = 'frame_0001.bmp';
        }
        frameDir = path.join(displayDir, state.currentDir);//update because video directory changed
    }

    fs.writeFileSync(stateFilePath, JSON.stringify(state));

    return fs.readFileSync(path.join(frameDir, state.currentFrame));
}

function getSortedVideoDirs(displayDir) {
    return fs.readdirSync(displayDir).filter(dir => dir.endsWith('_bitmap_frames')).sort();
}

function getSortedFrames(frameDir) {
    return fs.readdirSync(frameDir).sort();
}

module.exports = {
    hasDisplayWithId,
    getNextFrame
};