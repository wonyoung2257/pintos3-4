/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "lib/kernel/bitmap.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in(struct page *page, void *kva);
static bool anon_swap_out(struct page *page);
static void anon_destroy(struct page *page);

int swap_cnt;
struct bitmap *swap_table;
#define SECTOR_PAGE_SIZE 8

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
		.swap_in = anon_swap_in,
		.swap_out = anon_swap_out,
		.destroy = anon_destroy,
		.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void vm_anon_init(void)
{
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1, 1);
	swap_cnt = (int)disk_size(swap_disk) / DISK_SECTOR_SIZE;

	swap_table = bitmap_create(swap_cnt);
}

/* Initialize the file mapping */
bool anon_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* page struct 안의 Union 영역은 현재 uninit page이다.
		 ANON page를 초기화해주기 위해 해당 데이터를 모두 0으로 초기화해준다.
		 Q. 이렇게 하면 Union 영역은 모두 다 0으로 초기화되나? -> 맞다. */
	struct uninit_page *uninit = &page->uninit;
	memset(uninit, 0, sizeof(struct uninit_page));

	/* Set up the handler */
	/* 이제 해당 페이지는 ANON이므로 operations도 anon으로 지정한다. */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	anon_page->swap_index = -1;

	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in(struct page *page, void *kva)
{
	printf("anon_swap_in\n");
	struct anon_page *anon_page = &page->anon;
	int start_index = anon_page->swap_index;
	for (int i = 0; i < SECTOR_PAGE_SIZE; i++)
	{
		disk_read(swap_disk, start_index + (DISK_SECTOR_SIZE * i), page->frame->kva + (DISK_SECTOR_SIZE * i));
		bitmap_set(swap_table, start_index + i, false);
	}
	pml4_set_page(thread_current()->pml4, page->va, page->frame->kva, page->writable);
	anon_page->swap_index = -1;

	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out(struct page *page)
{
	printf("anon_swap_out\n");
	struct anon_page *anon_page = &page->anon;
	int bit_index = bitmap_scan_and_flip(swap_table, 0, 1, 0);
	if (bit_index == BITMAP_ERROR)
		return false;
	anon_page->swap_index = bit_index;

	for (int i = 0; i < SECTOR_PAGE_SIZE; i++)
	{
		disk_write(swap_disk, bit_index + (DISK_SECTOR_SIZE * i), page->frame->kva + (DISK_SECTOR_SIZE * i));
		// bitmap_set(swap_table, bit_index + i, true);
	}
	// bitmap 바꾸기
	pml4_clear_page(thread_current()->pml4, page->va);

	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
}
