#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <errno.h>

#define CONFIG_FILE "config.txt"
#define ZONE_MAP_FILE "zone_map.txt"
#define API_URL "https://api.cloudflare.com/client/v4"

#define MAX_LEN 256

char API_KEY[MAX_LEN] = "";
char EMAIL[MAX_LEN] = "";

typedef struct {
    char domain[MAX_LEN];
    char zone_id[MAX_LEN];
    char record_id[MAX_LEN];
    int proxied;
    char ip_address[MAX_LEN];
} ZoneMap;

ZoneMap *zone_map = NULL;
int zone_map_size = 0;

struct memory {
    char *response;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *)userp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (!ptr) return 0;

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->response[mem->size] = '\0';

    return realsize;
}

int load_config() {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        perror("Failed to open configuration file");
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        if (key && value) {
            if (strcmp(key, "API_KEY") == 0) {
                strncpy(API_KEY, value, MAX_LEN - 1);
            } else if (strcmp(key, "EMAIL") == 0) {
                strncpy(EMAIL, value, MAX_LEN - 1);
            }
        }
    }
    fclose(file);

    if (!API_KEY[0] || !EMAIL[0]) {
        fprintf(stderr, "API_KEY and EMAIL must be set in config file\n");
        return 0;
    }
    return 1;
}

void load_zone_map() {
    FILE *file = fopen(ZONE_MAP_FILE, "r");
    if (!file) return;

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        ZoneMap zm = {0};
        if (sscanf(line, "%255s %255s %255s %d %255s", zm.domain, zm.zone_id, zm.record_id, &zm.proxied, zm.ip_address) == 5) {
            ZoneMap *new_map = realloc(zone_map, (zone_map_size + 1) * sizeof(ZoneMap));
            if (!new_map) {
                perror("Memory allocation error");
                fclose(file);
                return;
            }
            zone_map = new_map;
            zone_map[zone_map_size++] = zm;
        }
    }
    fclose(file);
}

void save_zone_map() {
    FILE *file = fopen(ZONE_MAP_FILE, "w");
    if (!file) {
        perror("Could not open zone map file for writing");
        return;
    }

    for (int i = 0; i < zone_map_size; i++) {
        fprintf(file, "%s %s %s %d %s\n", zone_map[i].domain, zone_map[i].zone_id, zone_map[i].record_id,
                zone_map[i].proxied, zone_map[i].ip_address);
    }
    fclose(file);
}

void update_zone_map(const char *domain, const char *zone_id, const char *record_id, int proxied, const char *ip_address) {
    for (int i = 0; i < zone_map_size; i++) {
        if (strcmp(zone_map[i].domain, domain) == 0) {
            if (strcmp(zone_map[i].zone_id, zone_id) != 0 || strcmp(zone_map[i].record_id, record_id) != 0) {
                strncpy(zone_map[i].zone_id, zone_id, MAX_LEN - 1);
                strncpy(zone_map[i].record_id, record_id, MAX_LEN - 1);
                zone_map[i].proxied = proxied;
                strncpy(zone_map[i].ip_address, ip_address, MAX_LEN - 1);
                save_zone_map();
            }
            return;
        }
    }

    ZoneMap *new_map = realloc(zone_map, (zone_map_size + 1) * sizeof(ZoneMap));
    if (!new_map) {
        perror("Memory allocation error");
        return;
    }
    zone_map = new_map;
    strncpy(zone_map[zone_map_size].domain, domain, MAX_LEN - 1);
    strncpy(zone_map[zone_map_size].zone_id, zone_id, MAX_LEN - 1);
    strncpy(zone_map[zone_map_size].record_id, record_id, MAX_LEN - 1);
    zone_map[zone_map_size].proxied = proxied;
    strncpy(zone_map[zone_map_size].ip_address, ip_address, MAX_LEN - 1);
    zone_map_size++;
    save_zone_map();
}

char *make_request(const char *url, const char *method, const char *payload) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    struct memory chunk = {NULL, 0};
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    char auth_header[MAX_LEN + 32];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", API_KEY);
    headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        if (strcmp(method, "PUT") == 0) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        }
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.response);
        chunk.response = NULL;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return chunk.response;
}

void list_zones() {
    remove(ZONE_MAP_FILE);
    free(zone_map);
    zone_map = NULL;
    zone_map_size = 0;

    char url[512];
    snprintf(url, sizeof(url), "%s/zones", API_URL);
    char *response = make_request(url, "GET", NULL);

    if (!response) return;

    cJSON *json = cJSON_Parse(response);
    if (!json) {
        free(response);
        return;
    }

    cJSON *result = cJSON_GetObjectItem(json, "result");
    if (!cJSON_IsArray(result)) {
        cJSON_Delete(json);
        free(response);
        return;
    }

    cJSON *zone;
    cJSON_ArrayForEach(zone, result) {
        cJSON *zone_id = cJSON_GetObjectItem(zone, "id");
        cJSON *zone_name = cJSON_GetObjectItem(zone, "name");
        if (!cJSON_IsString(zone_id) || !cJSON_IsString(zone_name)) continue;

        char record_url[512];
        snprintf(record_url, sizeof(record_url), "%s/zones/%s/dns_records", API_URL, zone_id->valuestring);
        char *records_response = make_request(record_url, "GET", NULL);
        if (!records_response) continue;

        cJSON *records_json = cJSON_Parse(records_response);
        free(records_response);
        if (!records_json) continue;

        cJSON *records_result = cJSON_GetObjectItem(records_json, "result");
        if (!cJSON_IsArray(records_result)) {
            cJSON_Delete(records_json);
            continue;
        }

        cJSON *record;
        cJSON_ArrayForEach(record, records_result) {
            cJSON *record_id = cJSON_GetObjectItem(record, "id");
            cJSON *name = cJSON_GetObjectItem(record, "name");
            cJSON *proxied = cJSON_GetObjectItem(record, "proxied");
            cJSON *content = cJSON_GetObjectItem(record, "content");

            if (cJSON_IsString(record_id) && cJSON_IsString(name)) {
                update_zone_map(name->valuestring, zone_id->valuestring, record_id->valuestring,
                                proxied ? proxied->valueint : 0,
                                content ? content->valuestring : "");
            }
        }
        cJSON_Delete(records_json);
    }
    cJSON_Delete(json);
    free(response);
}

void display_record(const char *domain) {
    for (int i = 0; i < zone_map_size; i++) {
        if (strcmp(zone_map[i].domain, domain) == 0) {
            printf("Domain: %s\nZone ID: %s\nRecord ID: %s\nProxied: %d\nIP: %s\n",
                   zone_map[i].domain, zone_map[i].zone_id, zone_map[i].record_id,
                   zone_map[i].proxied, zone_map[i].ip_address);
            return;
        }
    }
    printf("Domain not found.\n");
}

int main(int argc, char *argv[]) {
    if (!load_config()) return 1;

    load_zone_map();

    if (argc < 2) {
        printf("Usage: %s <command> [args]\n", argv[0]);
        printf("Commands:\n  list_zones\n  display_record <domain>\n");
        return 1;
    }

    if (strcmp(argv[1], "list_zones") == 0) {
        list_zones();
    } else if (strcmp(argv[1], "display_record") == 0 && argc == 3) {
        display_record(argv[2]);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
    }

    free(zone_map);
    return 0;
}