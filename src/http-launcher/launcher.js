const http = require('http');
const fs = require('fs');
const path = require('path');
const open = require('open');

// Get the directory where the executable is running from
const EXECUTABLE_DIR = path.dirname(process.execPath);
const PORT = 8000;

// Find the root directory by looking for fheroes2.js
function findRootDir(startDir) {
    let currentDir = startDir;
    while (currentDir !== path.parse(currentDir).root) {
        if (fs.existsSync(path.join(currentDir, 'fheroes2.js'))) {
            return currentDir;
        }
        currentDir = path.dirname(currentDir);
    }
    return startDir; // Fallback to start directory if not found
}

const ROOT_DIR = findRootDir(EXECUTABLE_DIR);
const GAME_DIR = ROOT_DIR;
const ASSETS_DIR = path.join(ROOT_DIR, 'files', 'emscripten');

// Function to check if port is in use
function isPortInUse(port) {
    return new Promise((resolve) => {
        const server = http.createServer();
        server.once('error', () => {
            resolve(true);
        });
        server.once('listening', () => {
            server.close();
            resolve(false);
        });
        server.listen(port);
    });
}

// Handle SIGINT (Ctrl+C) to clean up properly
process.on('SIGINT', () => {
    console.log('\nShutting down server...');
    process.exit(0);
});

async function startServer() {
    try {
        // Check if port is in use
        if (await isPortInUse(PORT)) {
            console.error('\nError: Port 8000 is already in use.');
            console.error('Please close any other running instances of fheroes2 and try again.\n');
            process.exit(1);
        }

        // Create HTTP server to serve the game files
        http.createServer((req, res) => {
            let filePath;
            if (req.url === '/') {
                filePath = path.join(ASSETS_DIR, 'index.html');
            } else if (req.url === '/fheroes2.jpeg') {
                filePath = path.join(ASSETS_DIR, 'fheroes2.jpeg');
            } else {
                filePath = path.join(GAME_DIR, req.url);
            }
            console.log(`Attempting to serve: ${filePath}`);

            fs.readFile(filePath, (err, data) => {
                if (err) {
                    console.error(`Error reading file ${filePath}: ${err.message}`);
                    res.writeHead(404);
                    return res.end('Not found');
                }
                res.writeHead(200);
                res.end(data);
            });
        }).listen(PORT, () => {
            console.log(`Running at http://localhost:${PORT}`);
            console.log(`Serving game files from: ${GAME_DIR}`);
            console.log(`Serving assets from: ${ASSETS_DIR}`);

            // Open the browser
            open(`http://localhost:${PORT}`).catch(err => {
                console.error('Failed to open browser:', err);
                console.log(`Please open your browser manually at http://localhost:${PORT}`);
            });
        });

    } catch (error) {
        console.error('Failed to start server:', error);
        process.exit(1);
    }
}

startServer();
