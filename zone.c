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

// Structure to hold domain-to-zone mappings
typedef struct {
    char domain[256];
    char zone_id[256];
} ZoneMap;

ZoneMap zone_map[100];
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
        char *zone_id = strtok(NULL, "\n");

        if (domain && zone_id) {
            strncpy(zone_map[zone_map_size].domain, domain, sizeof(zone_map[zone_map_size].domain) - 1);
            strncpy(zone_map[zone_map_size].zone_id, zone_id, sizeof(zone_map[zone_map_size].zone_id) - 1);
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
        fprintf(file, "%s %s\n", zone_map[i].domain, zone_map[i].zone_id);
    }

    fclose(file);
}

// Function to update the zone map with a new mapping
void update_zone_map(const char *domain, const char *zone_id) {
    for (int i = 0; i < zone_map_size; i++) {
        if (strcmp(zone_map[i].domain, domain) == 0) {
            if (strcmp(zone_map[i].zone_id, zone_id) != 0) {
                strncpy(zone_map[i].zone_id, zone_id, sizeof(zone_map[i].zone_id) - 1);
                save_zone_map();
            }
            return;
        }
    }

    // New entry
    strncpy(zone_map[zone_map_size].domain, domain, sizeof(zone_map[zone_map_size].domain) - 1);
    strncpy(zone_map[zone_map_size].zone_id, zone_id, sizeof(zone_map[zone_map_size].zone_id) - 1);
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
        curl_slist_free_all(headers);
    }

    return chunk.response;
}

// Function to list zones and update zone map
void list_zones() {
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
                    cJSON *id = cJSON_GetObjectItem(zone, "id");
                    cJSON *name = cJSON_GetObjectItem(zone, "name");

                    if (cJSON_IsString(id) && cJSON_IsString(name)) {
                        printf("Domain: %s, Zone ID: %s\n", name->valuestring, id->valuestring);
                        update_zone_map(name->valuestring, id->valuestring);
                    }
                }
            }
            cJSON_Delete(json);
        } else {
            fprintf(stderr, "Error parsing JSON response.\n");
        }
        free(response);
    }
}

int main(int argc, char *argv[]) {
    if (!load_config()) {
        return 1;
    }

    load_zone_map();

    if (argc < 2) {
        printf("Usage: ./cloudflare <command> [args]\n");
        printf("Commands:\n");
        printf("  list_zones\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "list_zones") == 0) {
        list_zones();
    } else {
        printf("Unknown command: %s\n", command);
    }

    return 0;
}
