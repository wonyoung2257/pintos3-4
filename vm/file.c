/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
// #include "userprog/process.c"
// #include "include/threads/vaddr.h"
// project 3
#define FISIZE  (1 << 12)
static struct disk *file_disk;

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);


static bool file_lazy_load_segment(struct page *page, void *aux);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
	file_disk = disk_get(0, 1);
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */

	struct uninit_page *uninit = &page->uninit;
	memset(uninit, 0, sizeof(struct uninit_page));

	page->operations = &file_ops;
	struct file_page *file_page = &page->file;
	list_init(&file_page->page_list);

	list_push_back(&thread_current()->mmap_list, &file_page->file_elem);
	
	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	list_remove(&file_page->file_elem);
	// 수상
	if (page->frame && pml4_is_dirty(&thread_current()->pml4, page->va)) {
		file_write(file_page->file, page->frame->kva, page->file_inf->read_bytes);
	}
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable, 
			struct file *file, off_t offset) {
	uint32_t read_bytes = length;
	uint32_t zero_bytes = FISIZE - length;
	
	while (read_bytes > 0 || zero_bytes > 0)
	{
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < FISIZE ? read_bytes : FISIZE;
		size_t page_zero_bytes = FISIZE - page_read_bytes;

		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		struct file_information *file_inf = (struct file_information *)malloc(sizeof(struct file_information));
		file_inf->file = file;
		file_inf->ofs = offset;
		file_inf->read_bytes = page_read_bytes;

		if (!vm_alloc_page_with_initializer(VM_FILE, addr,	writable, file_lazy_load_segment, file_inf))
			return NULL;

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += FISIZE;
		offset += page_read_bytes;
	}
	return addr;

}

static bool
file_lazy_load_segment(struct page *page, void *aux)
{
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */

	struct file *file = ((struct file_information *)aux)->file;
	off_t offset = ((struct file_information *)aux)->ofs;
	size_t page_read_bytes = ((struct file_information *)aux)->read_bytes;
	size_t page_zero_bytes = FISIZE - page_read_bytes;
	file_seek(file, offset); // file의 오프셋을 offset으로 바꾼다. 이제 offset부터 읽기 시작한다.

	/* 페이지에 매핑된 물리 메모리(frame, 커널 가상 주소)에 파일의 데이터를 읽어온다. */
	/* 제대로 못 읽어오면 페이지를 FREE시키고 FALSE 리턴 */
	if (file_read(file, page->frame->kva, page_read_bytes) != (int)page_read_bytes)
	{
		palloc_free_page(page->frame->kva);
		return false;
	}
	/* 만약 1페이지 못 되게 받아왔다면 남는 데이터를 0으로 초기화한다. */
	memset(page->frame->kva + page_read_bytes, 0, page_zero_bytes);

	return true;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	struct page *page = spt_find_page(&thread_current()->spt, addr);
	struct file_page *file_page UNUSED = &page->file;

	if (page->frame && pml4_is_dirty(&thread_current()->pml4, page->va)) {
		file_write(file_page->file, page->frame->kva, page->file_inf->read_bytes);
	}
}
