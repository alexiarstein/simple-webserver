#!/bin/bash

# Lexiserver 1.1 - Installation Script
# Author: Alexia Michelle <alexiarstein@aol.com>

# Check if the script is being run as root
if [ "$(whoami)" != "root" ]; then
    echo "Please run this script as root (sudo bash install.sh)"
    echo "Error: Insufficient permissions to create /opt/lexiserver"
    exit 1
else
    echo "Welcome to Lexiserver 1.0 Installation"
    echo "----------------------------------------"
    echo "This script will install Lexiserver to /opt/lexiserver"

    # Prompt user for confirmation
    read -p "Continue with the installation? (Y/N): " choice

    case $choice in
        [YySs])
            echo "Installing Lexiserver..."
            # Create directory if it doesn't exist
            sudo mkdir -p /opt/lexiserver || { echo "Error creating directory /opt/lexiserver"; exit 1; }

            # Copy files to installation directory
            sudo cp -r * /opt/lexiserver/ || { echo "Error copying files to /opt/lexiserver"; exit 1; }

            # Generate SSL certificates
            echo "Generating SSL certificates..."
            sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout /opt/lexiserver/server.key -out /opt/lexiserver/server.crt || { echo "Error generating SSL certificates"; exit 1; }

            # Append system information to 404.html
            uname -a | awk '{print $1,$7,$3,$8,$9}' >> /opt/lexiserver/404.html || { echo "Error updating 404.html"; exit 1; }
            echo -e "</body>\n</html>" >> /opt/lexiserver/404.html
            echo "Installation complete!"
            echo "To uninstall, remove /opt/lexiserver"
            ;;
        [Nn])
            echo "Installation aborted"
            ;;
        *)
            echo "Invalid choice. Installation aborted."
            ;;
    esac
fi

	
