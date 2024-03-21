enum Action
{
    SORT,
    MERGE,
    WAITING,
    FINISH
};

struct distributor_args
{
    int sortType;
};

struct work_args
{
    int start;
    int size;
    int sortType;
    enum Action action;
};