#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_ITEMS 4
#define DEBUG_PRINTF printf("%s, at %i\n", __FILE__, __LINE__);

typedef enum {
    TYPE_NONE,
    TYPE_HUMAN,
    TYPE_SHEEP,
    TYPE_WOLF,
    TYPE_CABBAGE
} ITEM_TYPE;

typedef struct
{
    ITEM_TYPE type;
    int state;
} ITEM;

static ITEM *storyNew(void)
{
    ITEM *items;
    items = (ITEM *)malloc(sizeof(*items) * NUM_ITEMS);
    items[0].type = TYPE_HUMAN;
    items[1].type = TYPE_SHEEP;
    items[2].type = TYPE_WOLF;
    items[3].type = TYPE_CABBAGE;

    items[0].state = 0;
    items[1].state = 0;
    items[2].state = 0;
    items[3].state = 0;

    return items;
}

static int storyCheckConflict(const ITEM *items)
{
    if (items[1].state == items[3].state && items[1].state != items[0].state) // the sheep ate the cabbage!
    {
        return 1;
    }
    if (items[2].state == items[1].state && items[1].state != items[0].state) // the wolf ate the sheep!
    {
        return 1;
    }
    return 0;
}

static unsigned int storyCountProgress(const ITEM *items)
{
    unsigned int i, c;

    for (i = 0, c = 0; i < NUM_ITEMS; i += 1)
    {
        if (items[i].state == 0)
        {
            c += 1;
        }
    }

    return c;
}

static void storyEnd(ITEM *items)
{
    free(items);
}

static ITEM *storyDuplicate(const ITEM *items)
{
    ITEM *ret;

    ret = (ITEM *)malloc(sizeof(*ret) * NUM_ITEMS);
    memcpy(ret, items, sizeof(*ret) * NUM_ITEMS);
    return ret;
}

static unsigned int _indexOf(const ITEM *items, ITEM_TYPE type)
{
    unsigned int i;

    for (i = 0; i < NUM_ITEMS; i += 1)
    {
        if (items[i].type == type)
            return i;
    }

    return i;
}

static int storyCrossRiver(ITEM *items, ITEM_TYPE type)
{
    unsigned int iHuman, iType;

    iHuman = _indexOf(items, TYPE_HUMAN);
    iType = _indexOf(items, type);

    if (items[iHuman].state != items[iType].state) // They are on different side!
    {
        return 1;
    }

    switch (items[iHuman].state)
    {
    case 0:
        items[iType].state = 1;
        items[iHuman].state = 1;
        break;
    case 1:
        items[iType].state = 0;
        items[iHuman].state = 0;
        break;
    default:
        abort();
    }

    return storyCheckConflict(items);
}

typedef struct QUEUE_HISTORY_OBJ_STRUCT
{
    ITEM *items;
    struct QUEUE_HISTORY_OBJ_STRUCT *next;
} QUEUE_HISTORY_OBJ;

typedef struct QUEUE_STORY_STRUCT
{
    ITEM *items;
    struct QUEUE_STORY_STRUCT *next;
    QUEUE_HISTORY_OBJ *history;
    unsigned int progress;
    unsigned int nparents;
} QUEUE_STORY;

static QUEUE_STORY *queueCreate(void)
{
    QUEUE_STORY *ret;

    ret = (QUEUE_STORY *)malloc(sizeof(*ret));
    ret->items = storyNew();
    ret->progress = storyCountProgress(ret->items);
    ret->next = 0;
    ret->history = 0;
    ret->nparents = 0;

    return ret;
}

static QUEUE_STORY *queueInsert(QUEUE_STORY **head, ITEM *items, QUEUE_HISTORY_OBJ *history, unsigned int nparents)
{
    QUEUE_STORY *cur = *head;
    QUEUE_STORY *pre = cur;
    QUEUE_STORY *n = (QUEUE_STORY *)malloc(sizeof(*n));
    unsigned int p = storyCountProgress(items);
    unsigned int np = nparents + 1;

    n->items = items;
    n->progress = p;
    n->nparents = np;
    n->history = history;

    if (cur == 0)
    {
        // Insert at head
        n->next = *head;
        *head = n;
        return *head;
    }
    else if (p + np < cur->progress + cur->nparents)
    {
        // Insert at head
        n->next = *head;
        *head = n;
        return *head;
    }
    else
    {
        cur = cur->next;
        while (1)
        {
            if (cur == 0)
                break; // End
            if (pre->progress + pre->nparents <= p + np && p + np < cur->progress + cur->nparents)
                break; // Location found

            // Keep looking
            pre = cur;
            cur = cur->next;
        }

        // Insert
        pre->next = n;
        n->next = cur;
    }

    return *head;
}

static QUEUE_STORY *queueInsertNextSteps(QUEUE_STORY **head, ITEM *items, QUEUE_HISTORY_OBJ *history, unsigned int nparents)
{
    ITEM *d[4];
    QUEUE_HISTORY_OBJ *h = history;
    unsigned int i;

    for (i = 0; i < 4; i += 1)
        d[i] = storyDuplicate(items);

    if (storyCrossRiver(d[0], TYPE_SHEEP) == 1)
        storyEnd(d[0]);
    else
    {
        h = (QUEUE_HISTORY_OBJ *)malloc(sizeof(*h));
        h->items = items;
        h->next = history;
        queueInsert(head, d[0], h, nparents);
    }

    if (storyCrossRiver(d[1], TYPE_WOLF) == 1)
        storyEnd(d[1]);
    else
    {
        h = (QUEUE_HISTORY_OBJ *)malloc(sizeof(*h));
        h->items = items;
        h->next = history;
        queueInsert(head, d[1], h, nparents);
    }

    if (storyCrossRiver(d[2], TYPE_CABBAGE) == 1)
        storyEnd(d[2]);
    else
    {
        h = (QUEUE_HISTORY_OBJ *)malloc(sizeof(*h));
        h->items = items;
        h->next = history;
        queueInsert(head, d[2], h, nparents);
    }

    if (storyCrossRiver(d[3], TYPE_HUMAN) == 1)
        storyEnd(d[3]);
    else
    {
        h = (QUEUE_HISTORY_OBJ *)malloc(sizeof(*h));
        h->items = items;
        h->next = history;
        queueInsert(head, d[3], h, nparents);
    }

    return *head;
}

static QUEUE_STORY *queueRemoveHead(QUEUE_STORY **head, ITEM **items, QUEUE_HISTORY_OBJ **history, unsigned int *progress, unsigned int *nparents)
{
    QUEUE_STORY *next = (*head)->next;

    *items = (*head)->items;
    *history = (*head)->history;
    if (progress)
        *progress = (*head)->progress;
    if (nparents)
        *nparents = (*head)->nparents;
    free(*head);
    *head = next;

    return *head;
}

/*static void queueDestroy(QUEUE_STORY *head)
{
    QUEUE_STORY *next;

    while (head)
    {
        next = head->next;
        storyEnd(head->items);
        if (head->history)
            free(head->history);
        free(head);
        head = next;
    }
}*/

static void _reverse(QUEUE_HISTORY_OBJ *head, QUEUE_HISTORY_OBJ *pre)
{
    QUEUE_HISTORY_OBJ *next;

    if (head)
    {
        next = head->next;
        head->next = pre;
        _reverse(next, head);
    }
}

int main(void)
{
    QUEUE_STORY *q = queueCreate();
    ITEM *items, *final_item;
    QUEUE_HISTORY_OBJ *h, *cur;
    unsigned int p, np, i;

    while (1)
    {
        if (q == 0)
            break;
        queueRemoveHead(&q, &items, &h, &p, &np);
        if (p == 0)
            break;
        queueInsertNextSteps(&q, items, h, np);
    }

    if (p != 0)
    {
        return 1;
    }
    else
    {
        cur = h;
        while (cur->next)
        {
            cur = cur->next;
        }
        _reverse(h, 0);

        final_item = items;
        while (cur)
        {
            items = cur->items;

            for (i = 0; i < NUM_ITEMS; i += 1)
            {
                if (items[i].state == 0)
                {
                    switch (items[i].type)
                    {
                    case TYPE_HUMAN:
                        printf("¤H\t");
                        break;
                    case TYPE_SHEEP:
                        printf("¦Ï\t");
                        break;
                    case TYPE_WOLF:
                        printf("¯T\t");
                        break;
                    case TYPE_CABBAGE:
                        printf("µæ\t");
                        break;
                    default:
                        abort();
                    }
                }
                else
                    printf("\t");
            }
            printf("||\t");
            for (i = 0; i < NUM_ITEMS; i += 1)
            {
                if (items[i].state == 1)
                {
                    switch (items[i].type)
                    {
                    case TYPE_HUMAN:
                        printf("¤H\t");
                        break;
                    case TYPE_SHEEP:
                        printf("¦Ï\t");
                        break;
                    case TYPE_WOLF:
                        printf("¯T\t");
                        break;
                    case TYPE_CABBAGE:
                        printf("µæ\t");
                        break;
                    default:
                        abort();
                    }
                }
                else
                    printf("\t");
            }
            printf("\n\n");

            cur = cur->next;
        }

        items = final_item;
        for (i = 0; i < NUM_ITEMS; i += 1)
        {
            if (items[i].state == 0)
            {
                switch (items[i].type)
                {
                case TYPE_HUMAN:
                    printf("¤H\t");
                    break;
                case TYPE_SHEEP:
                    printf("¦Ï\t");
                    break;
                case TYPE_WOLF:
                    printf("¯T\t");
                    break;
                case TYPE_CABBAGE:
                    printf("µæ\t");
                    break;
                default:
                    abort();
                }
            }
            else
                printf("\t");
        }
        printf("||\t");
        for (i = 0; i < NUM_ITEMS; i += 1)
        {
            if (items[i].state == 1)
            {
                switch (items[i].type)
                {
                case TYPE_HUMAN:
                    printf("¤H\t");
                    break;
                case TYPE_SHEEP:
                    printf("¦Ï\t");
                    break;
                case TYPE_WOLF:
                    printf("¯T\t");
                    break;
                case TYPE_CABBAGE:
                    printf("µæ\t");
                    break;
                default:
                    abort();
                }
            }
            else
                printf("\t");
        }
        printf("\n\n");
        return 0;
    }
}