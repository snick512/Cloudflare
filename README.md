# Cloudflare
 A Cloudflare tool to manage the basics of zones using their direct API.

 The .sh is for a simple, terminal based interface style, and remains open until terminated. 

 Contributions welcome!

 # Running and Compiling
 config.txt
 ```ini
API_KEY=apikey
EMAIL=email
```

 Setup on Ubuntu 20.04, required (or will be):
 `libcurl4-openssl-dev libcjson-dev`

 ```bash 
 gcc -o cloudflare base.c -lcurl -lcjson
 gcc -o map zone.c -lcurl -lcjson
 ```

```bash
chmod +x cloudflare map cloudflare.sh
```

 ## Running
`zone_map.txt` references Zone/Record IDs. 

To store record IDs:
```bash
./map list_zones
```

To retrieve IDs:
```bash
$ ./map display_record wiki.wvpirates.org
Domain/Subdomain: wiki.wvpirates.org
Zone ID: x
Record ID: x
Proxied: 1
IP: x
$ 
```
## The .sh
cloudflare.sh is a (right now) simple, persisting way to add/delete records. 
```bash
./cloudflare.sh
```

 You may use the binary directly:
- List zones:
    `./cloudflare list_zones`
- Add/update a DNS record:
    `./cloudflare add_update_record <zone_id> A example.com 192.0.2.1 3600 1`
    
- Delete a DNS record:
    `./cloudflare delete_record <zone_id> <record_id>`
    
- Purge cache:
    `./cloudflare purge_cache <zone_id>`

# API Token permissions
[Creating an API Token](https://developers.cloudflare.com/fundamentals/api/get-started/create-token/)

```
Zone: Read (for listing zones)
DNS: Read (for reading DNS records)
DNS: Edit (for updating or adding DNS records)
Zone: Settings (if you need to modify zone settings, like enabling/disabling proxying)
```

### Ideas: 
- Map Zone IDs to domains automatically. 