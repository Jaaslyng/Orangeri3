#!/bin/bash

# Define source and destination paths
SRC="/home/jesper/Documents/Orangeri3/web"
DEST="/var/www/html"

# Ensure the destination folder exists
sudo mkdir -p $DEST

# Copy files, preserving permissions
sudo rsync -av --delete $SRC/ $DEST/

# Set correct ownership and permissions (adjust as needed)
sudo chown -R www-data:www-data $DEST
sudo chmod -R 755 $DEST

echo "Deployment completed successfully!"
