#include <assert.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslimits.h>
#include <unistd.h>

__attribute__((noreturn))
__attribute__((__format__(__printf__, 1, 0))) static void
fail(const char *error, ...) {
  va_list args;
  va_start(args, error);
  vfprintf(stderr, error, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

static int (*extract)(const char *cache_path, const char *output_path,
                      void (^progress)(int, int));

static int get_library_path(const char *candidate, char *output) {
  if (candidate) {
    if (access(candidate, R_OK) != 0) {
      fail("error: dsc_extractor.bundle not found at provided path: %s\n",
           candidate);
    }

    strncpy(output, candidate, PATH_MAX);
    return 0;
  }

  FILE *pipe = popen("xcrun --sdk iphoneos --show-sdk-platform-path", "r");
  if (pipe && fgets(output, PATH_MAX, pipe) != NULL) {
    output[strlen(output) - 1] = '\0';
    strcat(output, "/usr/lib/dsc_extractor.bundle");
    assert(pclose(pipe) == 0);
    return 0;
  }

  const char *builtin = "/usr/lib/dsc_extractor.bundle";
  if (access(builtin, R_OK) == 0) {
    strncpy(output, builtin, PATH_MAX);
    return 0;
  }

  return 1;
}

static int extract_shared_cache(const char *library_path,
                                const char *cache_path,
                                const char *output_path) {
  void *handle = dlopen(library_path, RTLD_LAZY);
  if (!handle)
    fail("error: failed to load bundle: %s\n", library_path);

  *(void **)(&extract) =
      dlsym(handle, "dyld_shared_cache_extract_dylibs_progress");

  if (!extract)
    fail("error: failed to load function from bundle: %s\n", library_path);

  return extract(cache_path, output_path, ^void(int completed, int total) {
    printf("extracted %d/%d\n", completed, total);
  });
}

int main(int argc, char *argv[]) {
  if (!(argc == 3 || argc == 4))
    fail("Usage: %s <shared-cache-path> <output-path> "
         "[<dsc_extractor.bundle-path>]\n",
         argv[0]);

  const char *shared_cache = argv[1];
  if (access(shared_cache, R_OK) != 0)
    fail("error: shared cache path doesn't exist: %s\n", shared_cache);

  const char *library_candidate = argc == 4 ? argv[3] : NULL;
  char library_path[PATH_MAX];
  if (get_library_path(library_candidate, library_path) != 0)
    fail("error: failed to fetch Xcode path\n");

  if (access(library_path, R_OK) != 0)
    fail("error: dsc_extractor.bundle wasn't found at expected path %s. "
         "Install Xcode or provide path as argument\n",
         library_path);
  printf("dsc_extractor.bundle found at %s\n", library_path);

  const char *output_path = argv[2];
  return extract_shared_cache(library_path, shared_cache, output_path);
}
