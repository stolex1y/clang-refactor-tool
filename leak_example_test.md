Запуск leak_example до рефакторинга (без виртуального деструктора):

```
=================================================================
==25==ERROR: AddressSanitizer: new-delete-type-mismatch on 0x6c208fde0010 in thread T0:
  object passed to delete has wrong type:
  size of the allocated type:   8 bytes;
  size of the deallocated type: 1 bytes.
    #0 0x70009146e3fb in operator delete(void*, unsigned long) ../../../../src/libsanitizer/asan/asan_new_delete.cpp:155
    #1 0x5d3aa92de28c in main /tmp/clang-refactor-tool/tests/tests_data/leak_example.cpp:18
    #2 0x700090da4577 in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58
    #3 0x700090da463a in __libc_start_main_impl ../csu/libc-start.c:360
    #4 0x5d3aa92de164 in _start (/tmp/clang-refactor-tool/cmake-build-debug/leak_example+0x1164) (BuildId: f880f451288207603880577553ece55e717412fe)

0x6c208fde0010 is located 0 bytes inside of 8-byte region [0x6c208fde0010,0x6c208fde0018)
allocated by thread T0 here:
    #0 0x70009146d2db in operator new(unsigned long) ../../../../src/libsanitizer/asan/asan_new_delete.cpp:86
    #1 0x5d3aa92de243 in main /tmp/clang-refactor-tool/tests/tests_data/leak_example.cpp:17
    #2 0x700090da4577 in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58
    #3 0x700090da463a in __libc_start_main_impl ../csu/libc-start.c:360
    #4 0x5d3aa92de164 in _start (/tmp/clang-refactor-tool/cmake-build-debug/leak_example+0x1164) (BuildId: f880f451288207603880577553ece55e717412fe)

SUMMARY: AddressSanitizer: new-delete-type-mismatch /tmp/clang-refactor-tool/tests/tests_data/leak_example.cpp:18 in main
==25==HINT: if you don't care about these errors you may set ASAN_OPTIONS=new_delete_type_mismatch=0
==25==ABORTING
```

Запуск утилиты для рефакторинга добавил `virtual` к деструктору:

```
/tmp/clang-refactor-tool/cmake-build-debug/leak_example.cpp:5:5: remark: Объявлен деструктор
5 |     ~Base() {}  // Невиртуальный
|     ^
```

После этого leak_example не содержал утечки, и ASan ничего не выдал.
