#!/usr/bin/env bash

echo "Syncing built files to target..."
SCRIPT_PATH=$(realpath "$0" | sed 's|\(.*\)/.*|\1|')
TEMP_FILE="$SCRIPT_PATH/.target"

# Function to get target configuration (username, IP, and password)
get_target_config() {
    if [[ -f $TEMP_FILE ]]; then
        source $TEMP_FILE
    else
        read -p "Enter target's username: " TARGET_USERNAME
        read -p "Enter target's IP address: " TARGET_IP
        read -sp "Enter target's password: " TARGET_PASSWORD
        echo
        # Save to temporary file (password encrypted using base64 for simplicity)
        echo "TARGET_USERNAME=$TARGET_USERNAME" > $TEMP_FILE
        echo "TARGET_IP=$TARGET_IP" >> $TEMP_FILE
        echo "TARGET_PASSWORD=$(echo $TARGET_PASSWORD | base64)" >> $TEMP_FILE
        chmod 600 $TEMP_FILE

        # Add host public key to target's authorized_keys
        echo "$TARGET_PASSWORD" | base64 --decode | sshpass -p "$(base64 --decode <<<"$TARGET_PASSWORD")" \
        ssh $TARGET_USERNAME@$TARGET_IP "mkdir -p ~/.ssh && touch ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys"
        cat ~/.ssh/id_rsa.pub | sshpass -p "$(base64 --decode <<<"$TARGET_PASSWORD")" \
        ssh $TARGET_USERNAME@$TARGET_IP 'cat >> .ssh/authorized_keys'
    fi
}

# Function to decode password
decode_password() {
    echo "$TARGET_PASSWORD" | base64 --decode
}

# Load or prompt for configuration
get_target_config

# Decode the password
TARGET_PASSWORD=$(decode_password)

rsync -avz --mkpath --rsync-path="sudo rsync" $SCRIPT_PATH/../build $TARGET_USERNAME@$TARGET_IP:/home/$TARGET_USERNAME/RpiSound/
rsync -avz --mkpath --rsync-path="sudo rsync" $SCRIPT_PATH/../sound $TARGET_USERNAME@$TARGET_IP:/home/$TARGET_USERNAME/RpiSound/build/
