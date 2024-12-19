#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define CONFIG_FILE "config.txt"    // Configuration file path
#define ZONE_MAP_FILE "zone_map.txt" // Zone map file path
#define API_URL "https://api.cloudflare.com/client/v4"

// Global variables for API credentials
char API_KEY[256] = "";
char EMAIL[256] = "";

// Structure to hold domain-to-zone mappings along with record ID, proxied status, and IP address
typedef struct {
    char domain[256];
    char zone_id[256];
    char record_id[256];
    int proxied; // 0 or 1 (not proxied or proxied)
    char ip_address[256];
} ZoneMap;

ZoneMap *zone_map = NULL;
int zone_map_size = 0;

// Helper structure for holding HTTP response
struct memory {
    char *response;
    size_t size;
};

// Write callback for libcurl
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *)userp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL) return 0; // Out of memory

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->response[mem->size] = '\0';

    return realsize;
}

// Function to load API key and email from configuration file
int load_config() {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open configuration file: %s\n", CONFIG_FILE);
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        if (key && value) {
            if (strcmp(key, "API_KEY") == 0) {
                strncpy(API_KEY, value, sizeof(API_KEY) - 1);
            } else if (strcmp(key, "EMAIL") == 0) {
                strncpy(EMAIL, value, sizeof(EMAIL) - 1);
            }
        }
    }

    fclose(file);

    if (strlen(API_KEY) == 0 || strlen(EMAIL) == 0) {
        fprintf(stderr, "Error: API_KEY and EMAIL must be set in the configuration file.\n");
        return 0;
    }

    return 1;
}

// Function to load zone map from file
void load_zone_map() {
    FILE *file = fopen(ZONE_MAP_FILE, "r");
    if (file == NULL) return;

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char *domain = strtok(line, " ");
        char *zone_id = strtok(NULL, " ");
        char *record_id = strtok(NULL, " ");
        char *proxied = strtok(NULL, " ");
        char *ip_address = strtok(NULL, "\n");

        if (domain && zone_id && record_id && proxied && ip_address) {
            zone_map = realloc(zone_map, (zone_map_size + 1) * sizeof(ZoneMap));
            if (zone_map == NULL) {
                fprintf(stderr, "Memory allocation error\n");
                return;
            }

            strncpy(zone_map[zone_map_size].domain, domain, sizeof(zone_map[zone_map_size].domain) - 1);
            strncpy(zone_map[zone_map_size].zone_id, zone_id, sizeof(zone_map[zone_map_size].zone_id) - 1);
            strncpy(zone_map[zone_map_size].record_id, record_id, sizeof(zone_map[zone_map_size].record_id) - 1);
            zone_map[zone_map_size].proxied = atoi(proxied);
            strncpy(zone_map[zone_map_size].ip_address, ip_address, sizeof(zone_map[zone_map_size].ip_address) - 1);
            zone_map_size++;
        }
    }

    fclose(file);
}

// Function to save zone map to file
void save_zone_map() {
    FILE *file = fopen(ZONE_MAP_FILE, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open zone map file for writing.\n");
        return;
    }

    for (int i = 0; i < zone_map_size; i++) {
        fprintf(file, "%s %s %s %d %s\n", zone_map[i].domain, zone_map[i].zone_id, zone_map[i].record_id, zone_map[i].proxied, zone_map[i].ip_address);
    }

    fclose(file);
}

// Function to update the zone map with a new mapping
void update_zone_map(const char *domain, const char *zone_id, const char *record_id, int proxied, const char *ip_address) {
    for (int i = 0; i < zone_map_size; i++) {
        if (strcmp(zone_map[i].domain, domain) == 0) {
            if (strcmp(zone_map[i].zone_id, zone_id) != 0) {
                strncpy(zone_map[i].zone_id, zone_id, sizeof(zone_map[i].zone_id) - 1);
                strncpy(zone_map[i].record_id, record_id, sizeof(zone_map[i].record_id) - 1);
                zone_map[i].proxied = proxied;
                strncpy(zone_map[i].ip_address, ip_address, sizeof(zone_map[i].ip_address) - 1);
                save_zone_map();
            }
            return;
        }
    }

    // New entry
    zone_map = realloc(zone_map, (zone_map_size + 1) * sizeof(ZoneMap));
    if (zone_map == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return;
    }
    strncpy(zone_map[zone_map_size].domain, domain, sizeof(zone_map[zone_map_size].domain) - 1);
    strncpy(zone_map[zone_map_size].zone_id, zone_id, sizeof(zone_map[zone_map_size].zone_id) - 1);
    strncpy(zone_map[zone_map_size].record_id, record_id, sizeof(zone_map[zone_map_size].record_id) - 1);
    zone_map[zone_map_size].proxied = proxied;
    strncpy(zone_map[zone_map_size].ip_address, ip_address, sizeof(zone_map[zone_map_size].ip_address) - 1);
    zone_map_size++;
    save_zone_map();
}

// Function to make HTTP requests
char *make_request(const char *url, const char *method, const char *payload) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    struct memory chunk = {NULL, 0};

    curl = curl_easy_init();
    if (curl) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");

        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", API_KEY);
        headers = curl_slist_append(headers, auth_header);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
            if (strcmp(method, "PUT") == 0) {
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            }
        } else if (strcmp(method, "DELETE") == 0) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    return chunk.response;
}

// Function to list zones
void list_zones() {
    // Delete the existing zone_map.txt file to ensure all new records are input
    if (remove(ZONE_MAP_FILE) == 0) {
        printf("Deleted existing zone_map.txt to input all new records.\n");
    } else {
        printf("zone_map.txt does not exist or could not be deleted. Proceeding to fetch records.\n");
    }

    // Reset the in-memory zone map
    free(zone_map);
    zone_map = NULL;
    zone_map_size = 0;

    char url[512];
    snprintf(url, sizeof(url), "%s/zones", API_URL);
    char *response = make_request(url, "GET", NULL);

    if (response) {
        cJSON *json = cJSON_Parse(response);
        if (json) {
            cJSON *result = cJSON_GetObjectItem(json, "result");
            if (cJSON_IsArray(result)) {
                cJSON *zone;
                cJSON_ArrayForEach(zone, result) {
                    cJSON *zone_id = cJSON_GetObjectItem(zone, "id");
                    cJSON *zone_name = cJSON_GetObjectItem(zone, "name");

                    if (cJSON_IsString(zone_id) && cJSON_IsString(zone_name)) {
                        // Fetch DNS records for the zone
                        char record_url[512];
                        snprintf(record_url, sizeof(record_url), "%s/zones/%s/dns_records", API_URL, zone_id->valuestring);
                        char *records_response = make_request(record_url, "GET", NULL);

                        if (records_response) {
                            cJSON *records_json = cJSON_Parse(records_response);
                            if (records_json) {
                                cJSON *records_result = cJSON_GetObjectItem(records_json, "result");
                                if (cJSON_IsArray(records_result)) {
                                    cJSON *record;
                                    cJSON_ArrayForEach(record, records_result) {
                                        cJSON *record_id = cJSON_GetObjectItem(record, "id");
                                        cJSON *name = cJSON_GetObjectItem(record, "name");
                                        cJSON *proxied = cJSON_GetObjectItem(record, "proxied");
                                        cJSON *content = cJSON_GetObjectItem(record, "content");

                                        if (cJSON_IsString(record_id) && cJSON_IsString(name)) {
                                            printf("Domain/Subdomain: %s, Zone ID: %s, Record ID: %s, Proxied: %d, IP: %s\n",
                                                   name->valuestring, zone_id->valuestring, record_id->valuestring,
                                                   proxied ? proxied->valueint : 0, content ? content->valuestring : "N/A");
                                            update_zone_map(name->valuestring, zone_id->valuestring, record_id->valuestring,
                                                            proxied ? proxied->valueint : 0, content ? content->valuestring : "");
                                        }
                                    }
                                }
                                cJSON_Delete(records_json);
                            }
                            free(records_response);
                        }
                    }
                }
            }
            cJSON_Delete(json);
        }
        free(response);
    }
}


// Function to display record details for a domain/subdomain
void display_record(const char *domain) {
    for (int i = 0; i < zone_map_size; i++) {
        if (strcmp(zone_map[i].domain, domain) == 0) {
            printf("Domain/Subdomain: %s\n", domain);
            printf("Zone ID: %s\n", zone_map[i].zone_id);
            printf("Record ID: %s\n", zone_map[i].record_id);
            printf("Proxied: %d\n", zone_map[i].proxied);
            printf("IP: %s\n", zone_map[i].ip_address);
            return;
        }
    }
    printf("Domain/Subdomain not found.\n");
}

int main(int argc, char *argv[]) {
    if (!load_config()) {
        return 1;
    }

    load_zone_map();

    if (argc < 2) {
        printf("Usage: ./map <command> [args]\n");
        printf("Commands:\n");
        printf("  list_zones\n");
        printf("  display_record <domain/subdomain>\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "list_zones") == 0) {
        list_zones();
    } else if (strcmp(command, "display_record") == 0 && argc == 3) {
        display_record(argv[2]);
    } else {
        printf("Unknown command: %s\n", command);
    }

    return 0;
}
