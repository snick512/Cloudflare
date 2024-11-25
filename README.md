# Cloudflare
 A Cloudflare tool to manage the basics of zones using their direct API.

 The .sh is for a simple, terminal based interace style, and remains open until terminated. 

 Or, you may use bin/cloudflare (a binary) directly. 

 Contributions welcome!

 # Compiling
 Setup on Ubuntu 20.04, required (or will be):
 `libcurl4-openssl-dev libcjson-dev`

 ```bash 
 gcc -o cloudflare base.c -lcurl -lcjson
 ```

 # Running
`./cloudflare.sh`

 You may use the binary directly:
- List zones:
    `./cloudflare list_zones`
- Add/update a DNS record:
    `./cloudflare add_update_record <zone_id> A example.com 192.0.2.1 3600 1`
    
- Delete a DNS record:
    `./cloudflare delete_record <zone_id> <record_id>`
    
- Purge cache:
    `./cloudflare purge_cache <zone_id>`

### Ideas: 
- Map Zone IDs to domains automatically. 