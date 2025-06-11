#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <iostream>


void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _num_meta_data_bytes();
size_t _size_meta_data();

int cookie = rand();
int order_array_initialized = 0;

struct MallocMetadata
{
    int cookie;
    size_t size;
    bool is_free ;
    MallocMetadata* prev;
    MallocMetadata* next;
};
typedef struct MallocMetadata *Mallocmetadata;

Mallocmetadata orders_array[11];

struct listMmap
{
    Mallocmetadata first = NULL;
    bool list_map_initialized = false;
};

typedef struct listMmap *listmmap;

listMmap map_list;

//void updateOrdersArray(MallocMetadata node,MallocMetadata orders_array);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t  _size_meta_data()
{
    return (sizeof(MallocMetadata));
}


size_t _num_free_blocks()
{
    size_t counter = 0;
    for(int i=0 ; i<11 ; i++)
    {
        MallocMetadata* temp_matadata = orders_array[i];
        while (temp_matadata != NULL)
        {
            if (temp_matadata->cookie != cookie)
            {
                exit(0xDEADBEEF);
            }
            if(temp_matadata->is_free == true)
            {
                counter++;
            }
            temp_matadata=temp_matadata->next;
        }
    }
    return counter;
}


size_t _num_free_bytes()
{
    size_t sum = 0;
    for(int i=0 ; i<11 ; i++)
    {
        MallocMetadata* temp_matadata = orders_array[i];
        while (temp_matadata != NULL)
        {
            if (temp_matadata->cookie != cookie)
            {
                exit(0xDEADBEEF);
            }

            if(temp_matadata->is_free == true)
            {
                sum = sum+temp_matadata->size;
            }
            temp_matadata=temp_matadata->next;
        }
    }
    return sum;
}

size_t _num_allocated_blocks()
{
    size_t counter = 0;
    for(int i=0 ; i<11 ; i++)
    {
        MallocMetadata *temp_matadata = orders_array[i];
        while (temp_matadata != NULL)
        {
            if (temp_matadata->cookie != cookie)
            {
                exit(0xDEADBEEF);
            }
            counter++;
            temp_matadata = temp_matadata->next;
        }
    }
    MallocMetadata *temp_matadata = map_list.first;

    while(temp_matadata != NULL)
    {
        if (temp_matadata->cookie != cookie)
        {
            exit(0xDEADBEEF);
        }
        counter++;
        temp_matadata = temp_matadata->next;
    }

    return counter;
}

size_t _num_allocated_bytes()
{
    size_t sum = 0;
    for(int i=0 ; i<11 ; i++)
    {
        MallocMetadata *temp_matadata = orders_array[i];
        while (temp_matadata != NULL)
        {
            if (temp_matadata->cookie != cookie)
            {
                exit(0xDEADBEEF);
            }
            sum = sum + temp_matadata->size;
            temp_matadata = temp_matadata->next;
        }
    }
    MallocMetadata *temp_matadata = map_list.first;
    while(temp_matadata != NULL)
    {
        if (temp_matadata->cookie != cookie)
        {
            exit(0xDEADBEEF);
        }
        sum = sum + temp_matadata->size;
        temp_matadata = temp_matadata->next;
    }
    return sum;
}


size_t _num_meta_data_bytes()
{
    return  _num_allocated_blocks()*sizeof(MallocMetadata);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////


void* initMapList(size_t size)
{
    void* mmap_result = mmap(nullptr,size,PROT_READ | PROT_WRITE ,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
    if(mmap_result == ((void *) -1))
        return nullptr;

    Mallocmetadata to_add = (Mallocmetadata)mmap_result;
    to_add->cookie = cookie;
    to_add->size = size;
    to_add->is_free = false;
    to_add->next = NULL;
    to_add->prev = NULL;
    map_list.first = to_add;
    map_list.list_map_initialized = true;
    size_t the_size = (size_t)mmap_result + _size_meta_data();
    void* result = (void*)(the_size);
    return result;
}

bool initOrdersArray()
{
    void* result = sbrk(0);
    if(result == (void*)(-1))
    {
        return false;
    }
    unsigned long val = 32 * 128 * 1024;
    unsigned long  mod_res = (unsigned long)(result) % val;
    unsigned long sub_res = val - mod_res;

    void* result2 = sbrk(sub_res);
    if(result2 == (void*)(-1))
    {
        return false;
    }

    void* result3 = sbrk(val);
    if(result3 == (void*)(-1))
    {
        return false;
    }


    for (int index = 0; index <= 9 ; index++)
    {
        orders_array[index] = NULL;
    }
    Mallocmetadata temp;
    for(int index = 0; index < 32; index++)
    {
        temp = (Mallocmetadata)((size_t) result3 + index*128*1024);
        temp->cookie = cookie;
        switch(index) {
            case 0:
                temp->next = (Mallocmetadata) ((size_t) temp + 1024 * 128);
                temp->prev = NULL;
                break;
            case 31:
                temp->next = NULL;
                temp->prev = (Mallocmetadata)((size_t)temp - 1024 * 128);
                break;
            default:
                temp->next = (Mallocmetadata) ((size_t) temp + 1024 * 128);
                temp->prev = (Mallocmetadata)((size_t)temp - 1024 * 128);
                break;
        }
        temp->is_free = true;
        temp->size = (128*1024) - _size_meta_data();

    }

    orders_array[10] = (Mallocmetadata)(result3);
    return true;
}


void updateOrdersArray(Mallocmetadata &node, Mallocmetadata &orders_array) {
    if (node->prev == NULL && node->next != NULL) {
        orders_array = node->next;
        node->next->prev = NULL;
    } else if (node->next == NULL && node->prev != NULL) {
        node->prev->next = NULL;
    } else if (node->next != NULL && node->prev != NULL) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    } else if (node->next == NULL && node->prev == NULL) {
        orders_array = NULL;
    }
}


void mergeFunction(Mallocmetadata &node_oldp , Mallocmetadata& temp , int i,bool parameter)
{
    if (temp == NULL)
    {
        size_t offset = (_size_meta_data());
        Mallocmetadata merge_empty = Mallocmetadata((size_t) node_oldp + offset);

        merge_empty->cookie = cookie;
        merge_empty->is_free = parameter;
        merge_empty->size = (size_t) pow(2, i + 1) * 128 - _size_meta_data();
        orders_array[i + 1] = merge_empty;
        merge_empty->prev = NULL;
        merge_empty->next = NULL;
        node_oldp = merge_empty;

    }
    else
    {
        while (temp->next != NULL)
        {
            temp = temp->next;
        }

        size_t offset = ( _size_meta_data());
        Mallocmetadata merge_empty = Mallocmetadata((size_t) node_oldp + offset);

        merge_empty->cookie = cookie;
        merge_empty->is_free = parameter;
        merge_empty->size = (size_t) pow(2, i + 1) * 128 - _size_meta_data();
        temp->next = merge_empty;
        merge_empty->prev = temp;
        merge_empty->next = NULL;
        node_oldp = merge_empty;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void removeFromMMapList(Mallocmetadata node_to_remove)
{

    if(node_to_remove==map_list.first)
    {
        map_list.first = node_to_remove->next;
    }

    if(node_to_remove->next != NULL)
    {
        node_to_remove->next->prev = node_to_remove->prev;
    }

    if(node_to_remove->prev != NULL)
    {
        node_to_remove->prev->next = node_to_remove->next;
    }

}

void* mmapWorker(size_t the_size)
{
    if(!map_list.list_map_initialized)
    {
        return initMapList(the_size);
    }

    else
    {
        Mallocmetadata temp_matadata = map_list.first;
        Mallocmetadata temp_last= NULL;

        while(temp_matadata)
        {
            if (temp_matadata->cookie != cookie)
            {
                exit(0xDEADBEEF);
            }
            temp_last = temp_matadata;
            temp_matadata = temp_matadata->next;
        }

        void* map_result=mmap(nullptr,the_size,PROT_READ | PROT_WRITE ,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
        if(map_result == ((void *) -1))
        {
            return nullptr;
        }

        Mallocmetadata to_add = (Mallocmetadata)map_result;
        to_add->cookie = cookie;
        to_add->size = the_size;
        to_add->is_free = false;


        if(temp_last == NULL)
        {
            map_list.first = to_add;
        }
        else
        {
            temp_last->next = to_add;
        }
        to_add->prev=temp_last;

        to_add->next= NULL;

        size_t new_size = (size_t)map_result + _size_meta_data();
        void* res = (void*)(new_size);
        return res;
    }
}
void cmp_cookies()
{
    for (int i = 0; i < 11; i++)
    {
        MallocMetadata *temp_matadata = orders_array[i];
        while (temp_matadata != NULL)
        {
            if (temp_matadata->cookie != cookie)
            {
                exit(0xDEADBEEF);
            }
            temp_matadata = temp_matadata->next;
        }
    }
}


void* smalloc(size_t size)
{
    switch (order_array_initialized)
    {
        case 0:
            bool res;
            res = initOrdersArray();
            if (!res) {
                return NULL;
            }
            order_array_initialized++;
            break;
    }

   if (size == 0 || size > 100000000)
    {
        return NULL;
    }

    if (size >= 128 * 1024)
    {
        return mmapWorker(size);
    }

    if (order_array_initialized > 0)
    {
        cmp_cookies();

        double res1 = (double) size / 128;
        double res2 = (double) log10(res1);
        double res3 = (double) log10(2);
        int order = res2 / res3 + 1;

        if(order<0)
        {
            order=0;
        }

        Mallocmetadata curr = orders_array[order];
        while (curr != NULL)
        {
            if (curr->is_free == true)
            {
                curr->cookie = cookie;
                curr->is_free = false;
                return (void *) (((size_t) curr) + _size_meta_data());
            }
            curr = curr->next;
        }

        for (int i = order + 1; i <= 10; i++)
        {
            Mallocmetadata curr = orders_array[i];

            while (curr != NULL)
            {
                if (curr->is_free == true)
                {
                    for (int j = i; j >= order; j--)
                    {
                        if (j == i)
                        {
                            if(curr->next!=NULL)
                            {
                                curr->next->prev = curr->prev;
                            }
                            if(curr->prev!=NULL)
                            {
                                curr->prev->next = curr->next;
                            }
                            if(curr->prev == NULL)
                            {
                                orders_array[i]=curr->next;
                            }

                            continue;
                        }

                        if (j == order)
                        {
                            Mallocmetadata temp = orders_array[j];
                            if (temp == NULL)
                            {
                                Mallocmetadata to_add_full = curr;

                                to_add_full->cookie = cookie;
                                to_add_full->is_free = false;
                                to_add_full->size = (size_t) pow(2, j) * 128 - _size_meta_data();
                                orders_array[j] = to_add_full;

                                size_t offset = (curr->size + _size_meta_data());
                                Mallocmetadata to_add_empty = Mallocmetadata ( (size_t)curr + offset) ;

                                to_add_empty->cookie = cookie;
                                to_add_empty->is_free = true;
                                to_add_empty->size = (size_t) pow(2, j) * 128 - _size_meta_data();
                                to_add_full->next = to_add_empty;
                                to_add_full->prev = NULL;
                                to_add_empty->next = NULL;
                                to_add_empty->prev = to_add_full;

                                return (void *) (((size_t) to_add_full) + _size_meta_data());
                            }

                            else
                            {
                                while (temp->next != NULL)
                                {
                                    temp = temp->next;
                                }

                                Mallocmetadata half1_full = curr;
                                half1_full->cookie = cookie;
                                half1_full->is_free = false;
                                half1_full->size = (size_t) pow(2, j) * 128 - _size_meta_data();
                                temp->next = half1_full;
                                half1_full->prev = temp;

                                size_t offset = (curr->size + _size_meta_data());
                                Mallocmetadata half2_empty = Mallocmetadata ( (size_t)curr + offset) ;

                                half2_empty->cookie = cookie;
                                half2_empty->is_free = true;
                                half2_empty->size = (size_t) pow(2, j) * 128 - _size_meta_data();
                                half1_full->next = half2_empty;
                                half2_empty->prev = half1_full;
                                half2_empty->next = NULL;


                                return (void *) (((size_t) half1_full) + _size_meta_data());
                            }
                        }

                        else
                        {
                            Mallocmetadata temp = orders_array[j];

                            if (temp == NULL)
                            {
                                size_t offset = (curr->size + _size_meta_data())*(pow(0.5,(i-j)));
                                Mallocmetadata newmetadatatoadd = Mallocmetadata ( (size_t)curr + offset) ;
                                newmetadatatoadd->cookie = cookie;
                                newmetadatatoadd->is_free = true;
                                newmetadatatoadd->size = (size_t)pow(2,j)*128 - _size_meta_data();
                                orders_array[j] = newmetadatatoadd;
                                newmetadatatoadd->prev = NULL;
                                newmetadatatoadd->next = NULL;

                            } else
                            {
                                while (temp->next != NULL)
                                {
                                    temp = temp->next;
                                }

                                size_t offset = (curr->size + _size_meta_data())*0.5*(i-j);
                                Mallocmetadata half2_empty = Mallocmetadata ( (size_t)curr + offset) ;
                                half2_empty->cookie = cookie;
                                half2_empty->is_free = true;
                                half2_empty->size = (size_t)pow(2,j)*128 - _size_meta_data();
                                temp->next = half2_empty;
                                half2_empty->prev = temp;
                                half2_empty->next = NULL;
                            }
                        }
                    }
                }
                curr = curr->next;
            }
        }
    }
    return NULL;
}


void sfree(void* p)
{
    if(p == NULL)
    {
        return;
    }

    cmp_cookies();
    Mallocmetadata node_free = (Mallocmetadata)((size_t)p - _size_meta_data());

    double res1 = (double) (node_free->size + _size_meta_data()) / 128;
    double res2 = (double) log10(res1);
    double res3 = (double) log10(2);
    int order = res2 / res3 ;

    if(order<0)
    {
        order = 0;
    }
    if(node_free->is_free)
    {
        return;
    }

    if(node_free->size >= 128*1024)
    {
        removeFromMMapList(node_free);
        if(munmap(node_free, node_free->size+_size_meta_data()) == -1)
            exit(1);
        return;
    }


    node_free->is_free = true;
    int found_buddy = 0;
    for(int i = order ; i<10 ; i++)
    {
        size_t buddy = ((size_t) ((node_free->size+_size_meta_data()) ^ (size_t) p));

        Mallocmetadata temp_matadata2 = orders_array[i];
        while (temp_matadata2 != NULL)
        {
            if (((size_t)temp_matadata2 +  _size_meta_data()) == buddy)
            {
                found_buddy = 1;
                if(temp_matadata2->is_free == true)
                {
                    updateOrdersArray(node_free, orders_array[i]);
                    updateOrdersArray(temp_matadata2, orders_array[i]);

                    Mallocmetadata temp = orders_array[i+1];
                    mergeFunction(node_free,temp,i,true);
                }
            }
            temp_matadata2 = temp_matadata2->next;
        }
        if(found_buddy == 0)
        {
            return;
        }
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* scalloc(size_t num, size_t size)
{
    if (size == 0 || size > 100000000)
    {
        return NULL;
    }

    for(int i=0 ; i<11 ; i++)
    {
        MallocMetadata *temp_matadata = orders_array[i];
        while (temp_matadata != NULL)
        {
            if (temp_matadata->cookie != cookie)
            {
                exit(0xDEADBEEF);
            }
            temp_matadata = temp_matadata->next;
        }
    }
    void* result = smalloc(size * num);
    if(result == NULL)
    {
        return NULL;
    }
    memset(result,0,size * num);
    return result;
}



size_t theMin(size_t t1 , size_t t2)
{
    if(t1 < t2)
    {
        return t1;
    }
    return t2;
}


void* srealloc(void* oldp, size_t size)
{
    if (size == 0 || size > 100000000)
    {
        return NULL;
    }
    cmp_cookies();


    if(oldp != nullptr)
    {
        Mallocmetadata node_oldp = (Mallocmetadata)  ((size_t) oldp - _size_meta_data());
        size_t node_oldp_size = node_oldp->size;

        double res1 = (double) (node_oldp->size + _size_meta_data()) / 128;
        double res2 = (double) log10(res1);
        double res3 = (double) log10(2);
        int order = res2 / res3 ;

        double res4 = (double) (size) / 128;
        double res5 = (double) log10(res4);
        double res6 = (double) log10(2);
        int order_of_size = res5 / res6 +1;
        if(order_of_size<0)
        {
            order_of_size=0;
        }

        if (node_oldp_size >= 128 * 1024)
        {
            if (size == node_oldp->size)
            {
                node_oldp->is_free = false;
                return oldp;
            }
            else
            {
                void *result = mmapWorker(size);
                if (result == NULL)
                {
                    return NULL;
                }

                memmove(result, oldp, theMin(size, node_oldp_size));
                sfree(oldp);
                return result;
            }
        }
        else
        {
            if(size <= node_oldp->size)
            {
                node_oldp->is_free= false;
                return oldp;
            }
            size_t sum_sizes = node_oldp_size + _size_meta_data();

            int flag_merge_not_useful = 5;

            while(sum_sizes < size)
            {
                int found_buddy = 0;
                for (int i = order; i < order_of_size; i++)
                {
                    size_t buddy = ((size_t) ((sum_sizes) ^ (size_t) oldp));

                    Mallocmetadata temp_matadata2 = orders_array[i];
                    while (temp_matadata2 != NULL)
                    {
                        if (((size_t) temp_matadata2 + _size_meta_data()) == buddy)
                        {
                            found_buddy = 1;
                            if (temp_matadata2->is_free == false) {
                                flag_merge_not_useful = 10;
                                break;
                            }
                            if (temp_matadata2->is_free == true) {
                                sum_sizes += temp_matadata2->size + _size_meta_data();
                            }

                        }
                        temp_matadata2 = temp_matadata2->next;
                        if (flag_merge_not_useful == 10 || found_buddy == 1)
                        {
                            break;
                        }
                    }
                    if (flag_merge_not_useful == 10 || found_buddy == 0)
                    {
                        break;
                    }
                }
                if (flag_merge_not_useful == 10)
                {
                    break;
                }
                break;
            }

            if(sum_sizes < size)
            {
                void* result = smalloc(size);
                if(result == NULL)
                {
                    return NULL;
                }
                memmove(result, oldp , node_oldp_size);
                return result;
            }
            else
            {
                size_t sum_sizes_new = node_oldp_size + _size_meta_data();
                for (int i = order; i < order_of_size; i++)
                {
                    if (sum_sizes_new >= size)
                    {
                        break;
                    }
                    size_t buddy = ((size_t) ((sum_sizes_new) ^ (size_t) oldp));

                    Mallocmetadata temp_matadata2 = orders_array[i];

                    while (temp_matadata2 != NULL)
                    {
                        if (((size_t) temp_matadata2 + _size_meta_data()) == buddy)
                        {
                            if (temp_matadata2->is_free == true) {

                                updateOrdersArray(node_oldp, orders_array[i]);
                                updateOrdersArray(temp_matadata2, orders_array[i]);
                                Mallocmetadata temp = orders_array[i + 1];

                                if (i + 1 == order_of_size) {

                                    mergeFunction(node_oldp, temp, i, false);

                                    if (temp == NULL) {
                                        memmove((void *) (((size_t) node_oldp) + _size_meta_data()), oldp,
                                                node_oldp_size);
                                        return (void *) (((size_t) node_oldp) + _size_meta_data());

                                    } else {
                                        memmove((void *) (((size_t) node_oldp) + _size_meta_data()), oldp,
                                                node_oldp_size);
                                        return (void *) (((size_t) node_oldp) + _size_meta_data());
                                    }
                                }


                                    mergeFunction(node_oldp, temp, i, true);


                                //sum_sizes_new += temp_matadata2->size + _size_meta_data();
                            }
                        }
                        temp_matadata2 = temp_matadata2->next;
                    }
                }
                return nullptr;
            }
        }
    }
    return nullptr;
}

