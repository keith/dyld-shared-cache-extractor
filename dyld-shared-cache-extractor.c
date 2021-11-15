#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PATH_SIZE 200

static void (*extract)(const char *cache_path, const char *output_path,
                       void (^progress)(int, int));

static int get_library_path(char *output) {
  FILE *pipe = popen("xcrun --sdk iphoneos --show-sdk-platform-path", "r");
  if (!pipe)
    return 1;

  if (fgets(output, PATH_SIZE, pipe) == NULL)
    return 1;

  output[strlen(output) - 1] = '\0';
  strcat(output, "/usr/lib/dsc_extractor.bundle");

  return pclose(pipe);
}

__attribute__((noreturn))
__attribute__((__format__(__printf__, 1, 0))) static void
fail(const char *error, ...) {
  va_list args;
  va_start(args, error);
  vfprintf(stderr, error, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

static void extract_shared_cache(const char *library_path,
                                 const char *cache_path,
                                 const char *output_path) {
  void *handle = dlopen(library_path, RTLD_LAZY);
  if (!handle)
    fail("error: failed to load bundle: %s\n", library_path);

  *(void **)(&extract) =
      dlsym(handle, "dyld_shared_cache_extract_dylibs_progress");

  if (!extract)
    fail("error: failed to load function from bundle: %s\n", library_path);

  extract(cache_path, output_path, ^void(int completed, int total) {
    printf("extracted %d/%d\n", completed, total);
  });
}

int main(int argc, char *argv[]) {
  if (argc != 3)
    fail("Usage: %s <shared-cache-path> <output-path>\n", argv[0]);

  const char *shared_cache = argv[1];
  if (access(shared_cache, R_OK) != 0)
    fail("error: shared cache path doesn't exist: %s\n", shared_cache);

  char library_path[PATH_SIZE];
  if (get_library_path(library_path) != 0)
    fail("error: failed to fetch Xcode path\n");

  if (access(library_path, R_OK) != 0)
    fail("error: dsc_extractor.bundle wasn't found at expected path, Xcode "
         "might have changed this location: %s\n",
         library_path);

  const char *output_path = argv[2];
  extract_shared_cache(library_path, shared_cache, output_path);
  return 0;
}
