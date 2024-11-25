#!/bin/bash

# Wrapper for the Cloudflare C Program
PROGRAM="./cloudflare"  # Path to the compiled C program

function list_zones() {
    echo "Fetching zones..."
    $PROGRAM list_zones
}

function add_update_record() {
    echo "Enter Zone ID:"
    read -r zone_id
    echo "Enter Record Type (e.g., A, AAAA, CNAME):"
    read -r record_type
    echo "Enter Record Name (e.g., example.com):"
    read -r record_name
    echo "Enter Record Content (e.g., 192.0.2.1):"
    read -r record_content
    echo "Enter TTL (e.g., 3600):"
    read -r ttl
    echo "Enable Proxy? (1 for yes, 0 for no):"
    read -r proxied

    echo "Adding/Updating record..."
    $PROGRAM add_update_record "$zone_id" "$record_type" "$record_name" "$record_content" "$ttl" "$proxied"
}

function delete_record() {
    echo "Enter Zone ID:"
    read -r zone_id
    echo "Enter Record ID:"
    read -r record_id

    echo "Deleting record..."
    $PROGRAM delete_record "$zone_id" "$record_id"
}

function purge_cache() {
    echo "Enter Zone ID:"
    read -r zone_id

    echo "Purging cache..."
    $PROGRAM purge_cache "$zone_id"
}

# Main menu
while true; do
    echo ""
    echo "Cloudflare Management Script"
    echo "============================"
    echo "1) List Zones"
    echo "2) Add/Update DNS Record"
    echo "3) Delete DNS Record"
    echo "4) Purge Cache"
    echo "5) Exit"
    echo ""
    echo -n "Choose an option: "
    read -r option

    case $option in
        1)
            list_zones
            ;;
        2)
            add_update_record
            ;;
        3)
            delete_record
            ;;
        4)
            purge_cache
            ;;
        5)
            echo "Exiting. Goodbye!"
            exit 0
            ;;
        *)
            echo "Invalid option. Please try again."
            ;;
    esac
done
