#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include <gst/gst.h>

#define BASE_URL "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent"

// 응답 데이터를 저장할 구조체
typedef struct {
    char *data;
    size_t size;
} ResponseData;

// 응답 데이터를 처리하는 함수
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total_size = size * nmemb;
    ResponseData *res = (ResponseData *)userdata;

    char *temp = realloc(res->data, res->size + total_size + 1);
    if (!temp) return 0;
    
    res->data = temp;
    memcpy(res->data + res->size, ptr, total_size);
    res->size += total_size;
    res->data[res->size] = '\0';

    return total_size;
}

// Google Gemini API 호출 함수
char *summarize_text(const char *input_text) {
    CURL *curl;
    CURLcode res;
    ResponseData response = {NULL, 0};
    
    curl = curl_easy_init();
    if (!curl) return NULL;

    // 환경 변수에서 API 키 가져오기
    const char *api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Error: GEMINI_API_KEY environment variable not set.\n");
        return NULL;
    }

    // API Key를 포함한 URL 생성
    char url[512];
    snprintf(url, sizeof(url), "%s?key=%s", BASE_URL, api_key);

    // JSON 요청 본문 생성
    json_t *root = json_object();
    json_t *contents = json_array();
    json_t *content_item = json_object();
    json_t *request_parts = json_array();
    json_t *part_item = json_object();

    json_object_set_new(part_item, "text", json_string(input_text));
    json_array_append_new(request_parts, part_item);
    json_object_set_new(content_item, "parts", request_parts);
    json_array_append_new(contents, content_item);
    json_object_set_new(root, "contents", contents);

    char *json_data = json_dumps(root, 0);
    json_decref(root);
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(json_data);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "cURL request failed: %s\n", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    // JSON 응답에서 text 필드 추출
    json_error_t error;
    json_t *response_root = json_loads(response.data, 0, &error);
    free(response.data);

    if (!response_root) {
        fprintf(stderr, "JSON parsing error: %s\n", error.text);
        return NULL;
    }

    json_t *candidates = json_object_get(response_root, "candidates");
    if (!json_is_array(candidates)) {
        fprintf(stderr, "Invalid JSON structure: 'candidates' is not an array.\n");
        json_decref(response_root);
        return NULL;
    }

    json_t *first_candidate = json_array_get(candidates, 0);
    if (!json_is_object(first_candidate)) {
        fprintf(stderr, "Invalid JSON structure: First candidate is not an object.\n");
        json_decref(response_root);
        return NULL;
    }

    json_t *content = json_object_get(first_candidate, "content");
    if (!json_is_object(content)) {
        fprintf(stderr, "Invalid JSON structure: 'content' is not an object.\n");
        json_decref(response_root);
        return NULL;
    }

    json_t *parts = json_object_get(content, "parts");
    if (!json_is_array(parts)) {
        fprintf(stderr, "Invalid JSON structure: 'parts' is not an array.\n");
        json_decref(response_root);
        return NULL;
    }

    json_t *first_part = json_array_get(parts, 0);
    if (!json_is_object(first_part)) {
        fprintf(stderr, "Invalid JSON structure: First part is not an object.\n");
        json_decref(response_root);
        return NULL;
    }

    json_t *text = json_object_get(first_part, "text");
    if (!json_is_string(text)) {
        fprintf(stderr, "Invalid JSON structure: 'text' is not a string.\n");
        json_decref(response_root);
        return NULL;
    }

    const char *text_value = json_string_value(text);
    char *result = strdup(text_value);  // 결과를 복사하여 반환
    json_decref(response_root);

    return result;
}

int main() {
    char input_text[1024];

    // 사용자로부터 텍스트 입력 받기
    printf("Enter text to summarize: ");
    if (!fgets(input_text, sizeof(input_text), stdin)) {
        fprintf(stderr, "Failed to read input.\n");
        return 1;
    }

    // 입력 텍스트에서 개행 문자 제거
    input_text[strcspn(input_text, "\n")] = '\0';

    // 입력 유효성 검사
    if (strlen(input_text) == 0) {
        fprintf(stderr, "Error: Input text is empty.\n");
        return 1;
    }

    // Gemini API 호출
    char *summary = summarize_text(input_text);
    if (summary) {
        printf("\nSummarized Text:\n%s\n", summary);
        free(summary);
    } else {
        printf("Failed to get summary.\n");
    }

    return 0;
}
