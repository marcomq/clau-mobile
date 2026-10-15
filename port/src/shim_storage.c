/* NVS emulated as one file per key: <data_dir>/nvs/<namespace>/<key>, raw bytes. Part of glo-mobile, GPL-3.0. */
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include "shim_internal.h"

#define MAX_HANDLES 16

static char nvs_root[1024];
static char handle_ns[MAX_HANDLES][16];

void glo_nvs_set_root(const char *dir)
{
    memset(handle_ns, 0, sizeof handle_ns);
    snprintf(nvs_root, sizeof nvs_root, "%s/nvs", dir);
    mkdir(nvs_root, 0755);
}

static int key_path(nvs_handle_t h, const char *key, char *out, size_t len)
{
    if (h == 0 || h > MAX_HANDLES || !handle_ns[h - 1][0] || !key)
        return 0;
    snprintf(out, len, "%s/%s/%s", nvs_root, handle_ns[h - 1], key);
    return 1;
}

static esp_err_t write_key(nvs_handle_t h, const char *key, const void *data, size_t len)
{
    char path[1200];
    if (!key_path(h, key, path, sizeof path))
        return ESP_ERR_NVS_INVALID_HANDLE;
    FILE *f = fopen(path, "wb");
    if (!f)
        return ESP_FAIL;
    size_t n = fwrite(data, 1, len, f);
    fclose(f);
    return n == len ? ESP_OK : ESP_FAIL;
}

/* out == NULL queries the stored length into *len. */
static esp_err_t read_key(nvs_handle_t h, const char *key, void *out, size_t *len)
{
    char path[1200];
    if (!key_path(h, key, path, sizeof path))
        return ESP_ERR_NVS_INVALID_HANDLE;
    FILE *f = fopen(path, "rb");
    if (!f)
        return ESP_ERR_NVS_NOT_FOUND;
    fseek(f, 0, SEEK_END);
    size_t size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    esp_err_t rc = ESP_OK;
    if (out) {
        if (*len < size)
            rc = ESP_ERR_NVS_INVALID_LENGTH;
        else if (fread(out, 1, size, f) != size)
            rc = ESP_FAIL;
    }
    *len = size;
    fclose(f);
    return rc;
}

static esp_err_t read_exact(nvs_handle_t h, const char *key, void *out, size_t want)
{
    size_t len = want;
    char tmp[8];
    esp_err_t rc = read_key(h, key, tmp, &len);
    if (rc != ESP_OK)
        return rc;
    if (len != want)
        return ESP_ERR_NVS_TYPE_MISMATCH;
    memcpy(out, tmp, want);
    return ESP_OK;
}

esp_err_t nvs_flash_init(void) { return nvs_root[0] ? ESP_OK : ESP_ERR_NVS_NOT_INITIALIZED; }

esp_err_t nvs_open(const char *name, nvs_open_mode_t mode, nvs_handle_t *out)
{
    (void)mode;
    if (!name || strlen(name) >= sizeof handle_ns[0])
        return ESP_ERR_NVS_INVALID_NAME;
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (!handle_ns[i][0]) {
            strcpy(handle_ns[i], name);
            char dir[1100];
            snprintf(dir, sizeof dir, "%s/%s", nvs_root, name);
            if (mkdir(dir, 0755) != 0 && errno != EEXIST)
                return ESP_FAIL;
            *out = (nvs_handle_t)(i + 1);
            return ESP_OK;
        }
    }
    return ESP_ERR_NVS_INVALID_HANDLE;
}

void nvs_close(nvs_handle_t h)
{
    if (h > 0 && h <= MAX_HANDLES)
        handle_ns[h - 1][0] = 0;
}

esp_err_t nvs_commit(nvs_handle_t h) { (void)h; return ESP_OK; }

esp_err_t nvs_erase_all(nvs_handle_t h)
{
    if (h == 0 || h > MAX_HANDLES || !handle_ns[h - 1][0])
        return ESP_ERR_NVS_INVALID_HANDLE;
    char cmd_dir[1100];
    snprintf(cmd_dir, sizeof cmd_dir, "%s/%s", nvs_root, handle_ns[h - 1]);
    DIR *d = opendir(cmd_dir);
    if (!d)
        return ESP_OK;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.')
            continue;
        char path[1400];
        snprintf(path, sizeof path, "%s/%s", cmd_dir, e->d_name);
        unlink(path);
    }
    closedir(d);
    return ESP_OK;
}

esp_err_t nvs_set_i8(nvs_handle_t h, const char *key, int8_t v) { return write_key(h, key, &v, sizeof v); }
esp_err_t nvs_set_i16(nvs_handle_t h, const char *key, int16_t v) { return write_key(h, key, &v, sizeof v); }
esp_err_t nvs_set_u64(nvs_handle_t h, const char *key, uint64_t v) { return write_key(h, key, &v, sizeof v); }
esp_err_t nvs_set_str(nvs_handle_t h, const char *key, const char *v) { return write_key(h, key, v, strlen(v) + 1); }
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *v, size_t len) { return write_key(h, key, v, len); }

esp_err_t nvs_get_i8(nvs_handle_t h, const char *key, int8_t *v) { return read_exact(h, key, v, sizeof *v); }
esp_err_t nvs_get_i16(nvs_handle_t h, const char *key, int16_t *v) { return read_exact(h, key, v, sizeof *v); }
esp_err_t nvs_get_u64(nvs_handle_t h, const char *key, uint64_t *v) { return read_exact(h, key, v, sizeof *v); }
esp_err_t nvs_get_str(nvs_handle_t h, const char *key, char *out, size_t *len) { return read_key(h, key, out, len); }
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *len) { return read_key(h, key, out, len); }
