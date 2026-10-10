// Checks mn_gzip.cpp against a real gzip-compressed ESPN answer:
//   g++ ... gz_test.cpp ../mini/mn_gzip.cpp -lz && ./a.out feed.json.gz feed.json
#include "../mini/mn_gzip.h"
#include <stdio.h>
struct FileSource : ByteSource {
  FILE* f;
  FileSource(const char* p) { f = fopen(p, "rb"); }
  int read() override { return f ? fgetc(f) : -1; }
  size_t readBytes(char* b, size_t n) override { return f ? fread(b, 1, n, f) : 0; }
};
int main(int, char** a) {
  FileSource in(a[1]);
  GzSource gz(in);
  FILE* ref = fopen(a[2], "rb");
  if (!gz.ok() || !ref) { puts("FAIL: open"); return 1; }
  long n = 0, bad = 0;
  char buf[777];
  for (;;) {
    size_t k = gz.readBytes(buf, sizeof(buf));
    if (!k) break;
    for (size_t i = 0; i < k; i++) if (fgetc(ref) != (uint8_t)buf[i]) bad++;
    n += k;
    if (n % 3 == 0 && gz.read() >= 0) { n++; if (fgetc(ref) < 0) bad++; }
  }
  printf("%ld bytes, %ld wrong, failed=%d, ref left=%d\n", n, bad, gz.failed(), fgetc(ref) != EOF);
  return bad || gz.failed();
}
