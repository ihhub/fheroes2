#!/bin/sh

npm run build
rm -rf ../android/app/src/main/assets/www
cp -R ./build/ ../android/app/src/main/assets/www
