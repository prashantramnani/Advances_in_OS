#include<stdio.h>
#include "disk.h"
#include "sfs.h"
#include <stdlib.h>
#include <string.h>

// void avaialble_inodes()
// {
// 	int new_file = create_file();
// 	if(new_file != -1)
// 	{
// 		avaialble_inodes();
// 		printf("%d ", new_file);
// 		remove_file(new_file);
// 	}
// }

// void avaialble_datanodes()
// {
// 	int new_dn = find_index_and_mark_bitmap_of_next_empty_datablock();
// 	if(new_dn != -1)
// 	{
// 		avaialble_datanodes();
// 		printf("%d ", new_dn);
// 		clear_bitmap_of_data_block_index(new_dn);
// 	}
// }

int main()
{	

	printf("\n1. Creating Disk with blocksize: %d\n", BLOCKSIZE);
	disk* diskptr = create_disk(BLOCKSIZE*21+24);
	printf("\n2. Size of created Disk: %d\n", diskptr->size);
	if(diskptr == NULL)
	{
		printf("Error in Creating disk\n");
		return -1;
	}

	printf("\n2. Checking Disk\n");
	char my_data[4] = {'A', 'B', 'C', '\0'};
	char buffer[BLOCKSIZE];
	memcpy(buffer, my_data, 4);
	write_block(diskptr, 0, buffer);


	read_block(diskptr, 0, buffer);
	char your_data[4];
	memcpy(your_data, buffer, 4);
	printf("Test data written: %s\n", my_data);
	printf("Data read: %s\n", your_data);

	printf("\n3. Formatting Disk\n");
	if(format(diskptr))
	{
		printf("Error in format\n");
		return -1;
	}

	printf("\n4. Mounting Disk\n");
	if(mount(diskptr))
	{
		printf("Error in mount\n");
		return -1;
	}
	print_superblock_info((super_block *)diskptr->block_arr[0]);
	printf("Mounted!\n");

	printf("\n5. Creating File\n");
	int inode_index = create_file();
	if(inode_index == -1)
	{
		printf("Error in Creating File!\n");
	}
	printf("Created file with inode = %d\n", inode_index);

	
	printf("\n6. Printing Stats of file with inode=%d just after creation\n", inode_index);
	stat(inode_index);


	int file_data_length = 320;
	char file_data_sent[file_data_length];
	for(int i = 0; i < file_data_length; i++ )
	{
		file_data_sent[i] = (i%128);
	}

	printf("\n7. Writing data to file\n");
	int length_written = write_i(inode_index, file_data_sent, file_data_length, 0);
	printf("Inode of the file: %d\n", inode_index);
	printf("Size of data sent: %d\n", file_data_length);
	printf("Size of data written: %d\n", length_written);

	printf("\n8. Printing Stats of file with inode=%d just after creation\n", inode_index);
	stat(inode_index);

	printf("\n9. Reading data from file\n");
	char file_data_received[file_data_length];
	int length_read = read_i(inode_index, file_data_received, file_data_length, 0);
	printf("Inode of the file: %d\n", inode_index);
	printf("Size of data written: %d\n",length_written);
	printf("Size of data read: %d\n", length_read);
	
	printf("\n10. Verifying read and written contents\n");
	int content_equal = (length_written == length_read);
	printf("Length is equal: %s\n", (content_equal ? "True" : "False"));

	for(int i = 0; (i < length_written) && content_equal; i++)
	{
		content_equal = content_equal && (file_data_sent[i] == file_data_received[i]); 
	}
	printf("Content is equal: %s\n", (content_equal ? "True" : "False"));

	printf("\n11. Removing File with inode=%d\n", inode_index);
	if(remove_file(inode_index))
	{
		printf("Error in Removing File\n");
	}
	else
	{
		printf("Successfully Removed File!\n");
	}

	printf("\n12. Printing Stats of file with inode=%d just after removal\n", inode_index);
	stat(inode_index);

	printf("\n13. Again Removing File with inode=%d\n", inode_index);
	if(remove_file(inode_index))
	{
		printf("Error in Removing File\n");
	}
	else
	{
		printf("Successfully Removed File!\n");
	}
	
	printf("\n14. Creating File\n");
	inode_index = create_file();
	if(inode_index == -1)
	{
		printf("Error in Creating File!\n");
	}
	printf("Created file with inode = %d\n", inode_index);
	
	printf("\n15. Printing Stats of file with inode=%d just after creation\n", inode_index);
	stat(inode_index);


	file_data_length = 1024;
	char file_data_sent_2[file_data_length];
	for(int i = 0; i < file_data_length; i++ )
	{
		file_data_sent_2[i] = (i%128);
	}

	printf("\n16. Writing data to file\n");
	length_written = write_i(inode_index, file_data_sent_2, file_data_length, 0);
	printf("Inode of the file: %d\n", inode_index);
	printf("Size of data sent: %d\n", file_data_length);
	printf("Size of data written: %d\n", length_written);

	printf("\n17. Printing Stats of file with inode=%d just after creation\n", inode_index);
	stat(inode_index);

	printf("\n18. Reading data from file\n");
	char file_data_received_2[file_data_length];
	length_read = read_i(inode_index, file_data_received_2, file_data_length, 0);
	printf("Inode of the file: %d\n", inode_index);
	printf("Size of data written: %d\n",length_written);
	printf("Size of data read: %d\n", length_read);
	
	printf("\n19. Verifying read and written contents\n");
	content_equal = (length_written == length_read);
	printf("Length is equal: %s\n", (content_equal ? "True" : "False"));

	for(int i = 0; (i < length_written) && content_equal; i++)
	{
		content_equal = content_equal && (file_data_sent_2[i] == file_data_received_2[i]); 
	}
	printf("Content is equal: %s\n", (content_equal ? "True" : "False"));

	printf("\n20. Removing File with inode=%d\n", inode_index);
	if(remove_file(inode_index))
	{
		printf("Error in Removing File\n");
	}
	else
	{
		printf("Successfully Removed File!\n");
	}

	printf("\n21. Printing Stats of file with inode=%d just after removal\n", inode_index);
	stat(inode_index);


	printf("\n22. Creating Directory\n");
	char* dirpath = "/hello/";
	if(create_dir(dirpath)< 0)
	{
		printf("Error in creation of directory %s!\n", dirpath);
	}
	else {
		printf("Successfully created directory %s!\n", dirpath);
	}

	char *test_string = "Hemlo! I am cheems!";
	char* filepath = "/hello/hello";
	printf("\n23. Writing the string \"%s\" to file: %s\n", test_string, filepath);	
	length_written = write_file(filepath, test_string, strlen(test_string) + 1, 0);
	printf("Number of bytes written: %d\n", length_written);

	printf("\n24. Reading from file: %s\n", filepath);
	char test_string_read[strlen(test_string) + 1];	
	length_read = read_file(filepath, test_string_read, strlen(test_string) + 1, 0);
	printf("Number of bytes read: %d\n", length_read);
	printf("Content read: %s\n", test_string_read);

	// printf("\n25. Checking avaialble inodes and datanodes\n");
	// printf("Avaiable inodes are: ");
	// avaialble_inodes();
	// printf("\nAvaiable datanodes are: ");
	// avaialble_datanodes();
	// printf("\n");

	printf("\n26. Deleting directory %s\n", dirpath);
	remove_dir(dirpath);


	// printf("\n27. Creating Another Directory\n");
	// dirpath = "/hello1/";
	// if(create_dir(dirpath)< 0)
	// {
	// 	printf("Error in creation of directory %s!\n", dirpath);
	// }
	// else {
	// 	printf("Successfully created directory %s!\n", dirpath);
	// }
	// printf("\n27. Checking avaialble inodes and datanodes\n");
	// printf("Avaiable inodes are: ");
	// avaialble_inodes();
	// printf("\nAvaiable datanodes are: ");
	// avaialble_datanodes();
	// printf("\n");
	free_disk(diskptr);
	printf("END\n");
	return 0;
}