#include "vcf_file_structure.h"

//-----------------------------------------------------
// Header/records allocation/freeing
//-----------------------------------------------------


vcf_header_entry_t* vcf_header_entry_new() {
    vcf_header_entry_t *entry = (vcf_header_entry_t*) malloc (sizeof(vcf_header_entry_t));
    entry->name = NULL;
    entry->num_keys = 0;
    entry->keys = (list_t*) malloc (sizeof(list_t));
    list_init("keys", 1, INT_MAX, entry->keys);
    entry->num_values = 0;
    entry->values = (list_t*) malloc (sizeof(list_t));
    list_init("values", 1, INT_MAX, entry->values);
    return entry;
}

void vcf_header_entry_free(vcf_header_entry_t *header_entry) {
    assert(header_entry);
    // Free entry name
    free(header_entry->name);
    // Free list of keys
    list_item_t* item = NULL;
    while ( (item = list_remove_item_async(header_entry->keys)) != NULL )  {
        free(item->data_p);
        list_item_free(item);
    }
    free(header_entry->keys);
    // Free list of values
    item = NULL;
    while ( (item = list_remove_item_async(header_entry->values)) != NULL ) {
        free(item->data_p);
        list_item_free(item);
    }
    free(header_entry->values);
    
    free(header_entry);
}

vcf_record_t* vcf_record_new() {
    vcf_record_t *record = (vcf_record_t*) calloc (1, sizeof(vcf_record_t));
    record->samples = array_list_new(16, 1.5, COLLECTION_MODE_ASYNCHRONIZED);
    return record;
}

void vcf_record_free(vcf_record_t *record) {
    assert(record);
    array_list_free(record->samples, free);
    free(record);
}

void vcf_record_free_deep(vcf_record_t *record) {
    assert(record);
    free(record->chromosome);
    free(record->id);
    free(record->reference);
    free(record->alternate);
    free(record->filter);
    free(record->info);
    free(record->format);
    array_list_free(record->samples, free);
    free(record);
}


//-----------------------------------------------------
// load data into the vcf_file_t
//-----------------------------------------------------

int add_vcf_header_entry(vcf_header_entry_t *header_entry, vcf_file_t *file) {
    assert(header_entry);
    assert(file);
    int result = array_list_insert(header_entry, file->header_entries);
//     if (result) {
//         (vcf_file->num_header_entries)++;
// //         LOG_DEBUG_F("header entry %zu\n", vcf_file->header_entries->size);
//     } else {
// //         LOG_DEBUG_F("header entry %zu not inserted\n", vcf_file->num_header_entries);
//     }
    return result;
}

int add_vcf_sample_name(char *name, int length, vcf_file_t *file) {
    assert(name);
    assert(file);
    int result = array_list_insert(strndup(name, length), file->samples_names);
//     if (result) {
//         (vcf_file->num_samples)++;
// //         LOG_DEBUG_F("sample %zu is %s\n", vcf_file->samples_names->size, name);
//     } else {
// //         LOG_DEBUG_F("sample %zu not inserted\n", vcf_file->num_samples);
//     }
    return result;
}

int add_vcf_batch(vcf_batch_t *batch, vcf_file_t *file) {
    assert(batch);
    assert(file);
    list_item_t *item = list_item_new(rand() % 1000, 1, batch); 
    list_insert_item(item, file->record_batches);
}

// int add_record(vcf_record_t* record, vcf_file_t *vcf_file) {
//     int result = array_list_insert(record, vcf_file->records);
// //     if (result) {
// //         (vcf_file->num_records)++;
// // //         LOG_DEBUG_F("record %zu\n", vcf_file->num_records);
// //     } else {
// // //         LOG_DEBUG_F("record %zu not inserted\n", vcf_file->num_records);
// //     }
//     return result;
// }

vcf_batch_t *fetch_vcf_batch(vcf_file_t *file) {
    assert(file);
    list_item_t *item = list_remove_item(file->record_batches);
    if (item) {
        vcf_batch_t *batch = item->data_p;
        list_item_free(item);
        return batch;
    }
    return NULL;
}



size_t get_num_vcf_header_entries(vcf_file_t *file) {
    assert(file);
    return file->header_entries->size;
}

size_t get_num_keys_in_vcf_header_entry(vcf_header_entry_t *entry) {
    assert(entry);
    return entry->keys->length;
}

size_t get_num_values_in_vcf_header_entry(vcf_header_entry_t *entry) {
    assert(entry);
    return entry->values->length;
}

size_t get_num_vcf_samples(vcf_file_t *file) {
    assert(file);
    return file->samples_names->size;
}

size_t get_num_vcf_records(vcf_file_t *file) {
    assert(file);
    size_t num_records = 0;
    for (list_item_t *i = file->record_batches->first_p; i != NULL; i = i->next_p) {
        num_records += file->record_batches->length;
    }
    return num_records;
}

size_t get_num_vcf_batches(vcf_file_t *file) {
    assert(file);
    return file->record_batches->length;
}


/* **************** Batch management functions **********************/



vcf_batch_t* vcf_batch_new(size_t size) {
    vcf_batch_t *vcf_batch = calloc (1, sizeof(vcf_batch_t));
    vcf_batch->text = NULL;
    if (size > 0) {
        vcf_batch->records = array_list_new(size, 1.2, COLLECTION_MODE_ASYNCHRONIZED);
    } else {
        vcf_batch->records = array_list_new(100, 1.2, COLLECTION_MODE_ASYNCHRONIZED);
    }
    
    return vcf_batch;
}

void vcf_batch_free(vcf_batch_t* batch) {
    assert(batch);
    
    if (batch->text && !mmap_vcf) {
//         printf("text to free = '%.*s'\n", 50, batch->text);
        free(batch->text);
    }
    array_list_free(batch->records, vcf_record_free);
    free(batch);
}

void add_record_to_vcf_batch(vcf_record_t *record, vcf_batch_t *batch) {
    assert(record);
    assert(batch);
    array_list_insert(record, batch->records);
}

inline int vcf_batch_is_empty(vcf_batch_t *batch) {
    assert(batch);
    return batch->records->size == 0;
}

inline int vcf_batch_is_full(vcf_batch_t *batch) {
    assert(batch);
//     printf("batch size = %zu\tcapacity = %zu\n", vcf_batch->size, vcf_batch->capacity);
    return batch->records->size == batch->records->capacity;
}

int vcf_batch_print(FILE *fd, vcf_batch_t *batch) {
    assert(fd);
    assert(batch);
    
    vcf_record_t *first_record = array_list_get(0, batch->records);
    if (first_record) {
        return fprintf(fd, "Batch with %zu/%zu records: begin in chr%s:%ld\n", 
                      batch->records->size, batch->records->capacity, 
                      first_record->chromosome, first_record->position);
    } else {
        return fprintf(fd, "Batch with %zu/%zu records (empty)\n", 
                      batch->records->size, batch->records->capacity); 
    }
}



/* ************ Header management functions **********************/

void set_file_format(char *fileformat, int length, vcf_file_t *file) {
    assert(fileformat);
    assert(file);
    file->format = fileformat;
    file->format_len = length;
//     LOG_DEBUG_F("set format = %s\n", file->format);
}

void set_header_entry_name(char *name, int length, vcf_header_entry_t *entry) {
    assert(name);
    assert(entry);
    entry->name = name;
    entry->name_len = length;
//     LOG_DEBUG_F("set name: %s\n", entry->name);
}

void add_header_entry_key(char *key, int length, vcf_header_entry_t *entry) {
    assert(key);
    assert(entry);
//     list_item_t *item = list_item_new(entry->num_keys, 1, key);
    list_item_t *item = list_item_new(entry->num_keys, 1, strndup(key, length));
    int result = list_insert_item(item, entry->keys);
    if (result) {
        entry->num_keys++;
//         LOG_DEBUG_F("key %zu = %s\n", entry->num_keys, (char*) item->data_p);
    } else {
//         LOG_DEBUG_F("key %zu not inserted\n", entry->num_keys);
    }
}

void add_header_entry_value(char *value, int length, vcf_header_entry_t *entry) {
    assert(value);
    assert(entry);
//     list_item_t *item = list_item_new(entry->num_values, 1, value);
    list_item_t *item = list_item_new(entry->num_values, 1, strndup(value, length));
    int result = list_insert_item(item, entry->values);
    if (result) {
        entry->num_values++;
//         LOG_DEBUG_F("value %zu = %s\n", entry->num_values, (char*) item->data_p);
    } else {
//         LOG_DEBUG_F("value %zu not inserted\n", entry->num_values);
    }
}


/* ************ Record management functions **********************/

void set_vcf_record_chromosome(char* chromosome, int length, vcf_record_t* record) {
    assert(chromosome);
    assert(record);
    
    if (starts_with_n(chromosome, "chrom", 5)) {
        record->chromosome = chromosome + 5;
        record->chromosome_len = length - 5;
    } else if (starts_with_n(chromosome, "chr", 3)) {
        record->chromosome = chromosome + 3;
        record->chromosome_len = length - 3;
    } else {
        record->chromosome = chromosome;
        record->chromosome_len = length;
    }
//     LOG_DEBUG_F("set chromosome: '%.*s'\n", record->chromosome_len, record->chromosome);
}

void set_vcf_record_position(long position, vcf_record_t* record) {
    assert(record);
    record->position = position;
//     LOG_DEBUG_F("set position: %ld\n", record->position);
}

void set_vcf_record_id(char* id, int length, vcf_record_t* record) {
    assert(id);
    assert(record);
    record->id = id;
    record->id_len = length;
//     LOG_DEBUG_F("set id: %s\n", record->id);
}

void set_vcf_record_reference(char* reference, int length, vcf_record_t* record) {
    assert(reference);
    assert(record);
    record->reference = reference;
    record->reference_len = length;
//     LOG_DEBUG_F("set reference: %s\n", record->reference);
}

void set_vcf_record_alternate(char* alternate, int length, vcf_record_t* record) {
    assert(alternate);
    assert(record);
    record->alternate = alternate;
    record->alternate_len = length;
//     LOG_DEBUG_F("set alternate: %s\n", record->alternate);
}

void set_vcf_record_quality(float quality, vcf_record_t* record) {
    assert(record);
    record->quality = quality;
//     LOG_DEBUG_F("set quality: %f\n", record->quality);
}

void set_vcf_record_filter(char* filter, int length, vcf_record_t* record) {
    assert(filter);
    assert(record);
    record->filter = filter;
    record->filter_len = length;
//     LOG_DEBUG_F("set filter: %s\n", record->filter);
}

void set_vcf_record_info(char* info, int length, vcf_record_t* record) {
    assert(info);
    assert(record);
    record->info = info;
    record->info_len = length;
//     LOG_DEBUG_F("set info: %s\n", record->info);
}

void set_vcf_record_format(char* format, int length, vcf_record_t* record) {
    assert(format);
    assert(record);
    record->format = format;
    record->format_len = length;
//     LOG_DEBUG_F("set format: %s\n", record->format);
}

void add_vcf_record_sample(char* sample, int length, vcf_record_t* record) {
    assert(sample);
    assert(record);
//     int result = array_list_insert(sample, record->samples);
    int result = array_list_insert(strndup(sample, length), record->samples);
//     if (result) {
//         LOG_DEBUG_F("sample %s inserted\n", sample);
//     } else {
//         LOG_DEBUG_F("sample %s not inserted\n", sample);
//     }
}
