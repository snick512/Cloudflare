#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define CONFIG_FILE "config.txt" // Configuration file path
#define API_URL "https://api.cloudflare.com/client/v4"

// Global variables for API credentials
char API_KEY[256] = "";
char EMAIL[256] = "";

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

    // Validate and return JSON
    if (chunk.response) {
        cJSON *json = cJSON_Parse(chunk.response);
        if (!json) {
            fprintf(stderr, "Error: Invalid JSON response: %s\n", chunk.response);
            free(chunk.response);
            return NULL;
        }
        cJSON_Delete(json);
    }

    return chunk.response;
}

// Function to list zones
void list_zones() {
    char url[512];
    snprintf(url, sizeof(url), "%s/zones", API_URL);

    char *response = make_request(url, "GET", NULL);
    if (response) {
        // Print JSON directly for `jq` to consume
        printf("%s\n", response);
        free(response);
    }
}


// Function to print JSON in a compact form for jq compatibility
void print_json(const char *json_str) {
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        fprintf(stderr, "Error: Invalid JSON format.\n");
        return;
    }

    char *formatted_json = cJSON_PrintUnformatted(json);
    if (formatted_json) {
        printf("%s\n", formatted_json);
        free(formatted_json);
    }

    cJSON_Delete(json);
}

// Modify add_update_record
void add_update_record(const char *zone_id, const char *type, const char *name, const char *content, int ttl, int proxied) {
    char fetch_url[512];
    snprintf(fetch_url, sizeof(fetch_url), "%s/zones/%s/dns_records?type=%s&name=%s", API_URL, zone_id, type, name);

    char *fetch_response = make_request(fetch_url, "GET", NULL);
    if (!fetch_response) {
        fprintf(stderr, "Error: Failed to fetch DNS records.\n");
        return;
    }

    // Validate JSON 
    /* Remove all but JSON */
    //printf("Fetched DNS Records:\n");
    print_json(fetch_response);

    cJSON *json = cJSON_Parse(fetch_response);
    if (!json) {
        fprintf(stderr, "Error: Failed to parse JSON response.\n");
        free(fetch_response);
        return;
    }

    cJSON *result = cJSON_GetObjectItem(json, "result");
    if (!result || !cJSON_IsArray(result)) {
        fprintf(stderr, "Error: Invalid response structure.\n");
        cJSON_Delete(json);
        free(fetch_response);
        return;
    }

    int record_exists = 0;
    cJSON *record;
    cJSON_ArrayForEach(record, result) {
        cJSON *record_name = cJSON_GetObjectItem(record, "name");
        cJSON *record_content = cJSON_GetObjectItem(record, "content");
        cJSON *record_proxied = cJSON_GetObjectItem(record, "proxied");
        cJSON *record_id = cJSON_GetObjectItem(record, "id");

        if (record_name && record_content && record_id && record_proxied && strcmp(record_name->valuestring, name) == 0) {
            if (strcmp(record_content->valuestring, content) == 0 && record_proxied->valueint == proxied) {
                /* Remove all but JSON */
               // printf("Record already exists. No changes made.\n");
                record_exists = 1;
                break;
            } else {
                const char *record_id_str = record_id->valuestring;
                /* Remove all but JSON */
               // printf("Deleting existing record (ID: %s):\n", record_id_str);

                char delete_url[512];
                snprintf(delete_url, sizeof(delete_url), "%s/zones/%s/dns_records/%s", API_URL, zone_id, record_id_str);

                char *delete_response = make_request(delete_url, "DELETE", NULL);
                if (delete_response) {
                    /* Remove all but JSON */
                    //printf("Delete Record Response:\n");
                    print_json(delete_response);
                    free(delete_response);
                }
            }
        }
    }

    cJSON_Delete(json);
    free(fetch_response);

    if (!record_exists) {
        char add_url[512], payload[1024];
        snprintf(add_url, sizeof(add_url), "%s/zones/%s/dns_records", API_URL, zone_id);

        snprintf(payload, sizeof(payload),
                 "{\"type\":\"%s\", \"name\":\"%s\", \"content\":\"%s\", \"ttl\":%d, \"proxied\":%s}",
                 type, name, content, ttl, proxied ? "true" : "false");

        char *add_response = make_request(add_url, "POST", payload);
        if (add_response) {
            /* Remove all but JSON */
           // printf("Add Record Response:\n");
            print_json(add_response);
            free(add_response);
        }
    }
}

// Modify delete_record
void delete_record(const char *zone_id, const char *record_id) {
    char url[512];
    snprintf(url, sizeof(url), "%s/zones/%s/dns_records/%s", API_URL, zone_id, record_id);

    char *response = make_request(url, "DELETE", NULL);
    if (response) {
       
       /* Remove all but JSON */
       // printf("Delete Record Response:\n");
        print_json(response);
        free(response);
    }
}




// Function to purge cache
void purge_cache(const char *zone_id) {
    char url[512], payload[64];
    snprintf(url, sizeof(url), "%s/zones/%s/purge_cache", API_URL, zone_id);
    snprintf(payload, sizeof(payload), "{\"purge_everything\":true}");

    char *response = make_request(url, "POST", payload);
    if (response) {
        printf("Purge Cache Response: %s\n", response);
        free(response);
    }
}

int main(int argc, char *argv[]) {
    if (!load_config()) {
        return 1;
    }

    if (argc < 2) {
        printf("Usage: ./cloudflare <command> [args]\n");
        printf("Commands:\n");
        printf("  list_zones\n");
        printf("  add_update_record <zone_id> <type> <name> <content> <ttl> <proxied>\n");
        printf("  delete_record <zone_id> <record_id>\n");
        printf("  purge_cache <zone_id>\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "list_zones") == 0) {
        list_zones();
    } else if (strcmp(command, "add_update_record") == 0) {
        if (argc != 8) {
            printf("Usage: ./cloudflare add_update_record <zone_id> <type> <name> <content> <ttl> <proxied>\n");
            return 1;
        }
        const char *zone_id = argv[2];
        const char *type = argv[3];
        const char *name = argv[4];
        const char *content = argv[5];
        int ttl = atoi(argv[6]);
        int proxied = atoi(argv[7]);
        add_update_record(zone_id, type, name, content, ttl, proxied);
    } else if (strcmp(command, "delete_record") == 0) {
        if (argc != 4) {
            printf("Usage: ./cloudflare delete_record <zone_id> <record_id>\n");
            return 1;
        }
        const char *zone_id = argv[2];
        const char *record_id = argv[3];
        delete_record(zone_id, record_id);
    } else if (strcmp(command, "purge_cache") == 0) {
        if (argc != 3) {
            printf("Usage: ./cloudflare purge_cache <zone_id>\n");
            return 1;
        }
        const char *zone_id = argv[2];
        purge_cache(zone_id);
    } else {
        printf("Unknown command: %s\n", command);
    }

    return 0;
}