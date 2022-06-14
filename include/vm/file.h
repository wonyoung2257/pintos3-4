#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

struct file_page
{
};

struct mmap_file
{
	int mapid;									/* 성공 시 리턴된 mapping id */
	struct file *file;					/* 매핑하는 파일의 파일 오브젝트 */
	struct list_elem mmap_elem; /* mmap_file들의 리스트 연결을 위한 구조체 */
	struct list page_list;			/* mmap_file에 해당하는 모든 page들의 리스트 */
};

void vm_file_init(void);
bool file_backed_initializer(struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
							struct file *file, off_t offset);
void do_munmap(void *va);
#endif
