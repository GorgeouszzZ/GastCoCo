#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string>
#include <chrono>
#include <mutex>
#include "../other/type.hpp"
#include "../other/graphIO.hpp"
#include "CBListChunkSize.hpp"
// #include "../other/futex.hpp"

//#pragma pack(1)

namespace GastCoCo
{

    enum class ComputeMode : int8_t
    {
        Pull,
        Push,
        Mixed
    };


    struct InterNode;
    struct Lv1Chunk;
    struct LeafChunk;
    union nextPointer
    {
        LeafChunk* nextLeafChunk;
        Lv1Chunk* nextLv1Chunk;
    };
    struct Node //size 21?-->24
    {
        uint32_t count;
        bool IsLeaf;
        InterNode* Parent;
        Node() :Parent(nullptr), IsLeaf(false), count(0) {};
        virtual ~Node() {};
        virtual void Print();
    };
    //
    struct alignas(CACHE_LINE_SIZE* CACHE_LINE_MULT) InterNode : Node
    {
        EdgeID KeyList[INTER_NODE_KEY_CNT];
        Node* Child[INTER_NODE_PTR_CNT];
        InterNode() :Node()
        {
            for (uint32_t i = 0;i < INTER_NODE_KEY_CNT;++i)
            {
                KeyList[i] = -1;
                Child[i] = nullptr;
            }
            Child[INTER_NODE_PTR_CNT - 1] = nullptr;
        };
        virtual ~InterNode() {};
        bool Insert(VertexID dest, Node* NewNode);
        int32_t Split(InterNode* Brother, int32_t& NewKey);
        void Print();
    };

    struct alignas(CACHE_LINE_SIZE* CACHE_LINE_MULT) LeafChunk : Node
    {
        AdjUnit NeighboorChunk[LEAF_CHUNK_NCNT];
        nextPointer nextPtr;
        int32_t nextType;//for next
        LeafChunk()
        {
            nextPtr.nextLeafChunk = nullptr;
            Parent = nullptr;
            count = 0;
            nextType = LEAFCHUNK_LEVEL;
            IsLeaf = true;
            for (uint32_t i = 0;i < LEAF_CHUNK_NCNT;++i)
            {
                NeighboorChunk[i] = { 0, 0 };
            }
        }
        virtual ~LeafChunk() {};
        bool Insert(AdjUnit NewNeighboor);
        uint32_t Split(LeafChunk* p);
        void Print();
    };

    class BplusTree
    {
    public:
        Node* Root;
        uint32_t LeafCnt;
        BplusTree() :Root(nullptr), LeafCnt(0) {};
        bool Insert(AdjUnit NewNeighboor);
        bool InsertKnowPos(AdjUnit NewNeighboor, LeafChunk* InsertPos);
        LeafChunk* Find(VertexID dest);
        bool Add_Node(InterNode* Parent, uint32_t NewKey, Node* NewNode);
        void Print();
        LeafChunk* GetLeafStart();
        LeafChunk* GetLeafEnd();
    }__attribute__((packed));

    //TODO: AOS/SOA
    struct alignas(CACHE_LINE_SIZE* CACHE_LINE_L1_MULT) Lv1Chunk
    {
        uint32_t count;
        int32_t nextType;//for next
        AdjUnit NeighboorChunk[LV1CHUNK_NCNT];
        nextPointer nextPtr;
        Lv1Chunk()
        {
            nextPtr.nextLeafChunk = nullptr;
            count = 0;
            nextType = LEAFCHUNK_LEVEL;
            for (uint32_t i = 0;i < LV1CHUNK_NCNT;++i)
            {
                NeighboorChunk[i] = { 0, 0 };
            }
        }
    };


    class CBLNode
    {
    public:
        uint32_t Level;
        uint32_t NeighboorCnt;
        uint32_t ChunkCnt;
        BplusTree BPT;
        nextPointer Neighboor;
        CBLNode();
        uint32_t Insert(AdjUnit NeighboorINFO);
    };

    class CBList
    {
    public:
        VertexID VertexNum;
        EdgeID EdgeNum;
        CBLNode* VertexTableOut;
        CBLNode* VertexTableIn;
        nextPointer GTChainOut;
        nextPointer GTChainIn;
        std::vector<std::mutex> writelockOut;
        std::vector<std::mutex> writelockIn;
        // std::vector<Futex> vertex_lock;
        std::vector<std::mutex> vertex_lock;
        class EdgeIterator
        {
        public:
            VertexID v;
            int level;
            int NeighboorCnt;
            int AccessEdgeCnt;
            nextPointer CurrentPtr;
            int CurrentPos;
            bool alive;
            EdgeIterator(const CBList* Graph, VertexID src);
            EdgeIterator operator++ ();
            AdjUnit GetEdegInfo();
        };
        CBList(std::string infopath, ComputeMode cm, bool load_graph_in_order);
        CBList(VertexID Node, EdgeID Edge, std::string path);
        CBList(VertexID Node, EdgeID Edge, std::string path, bool incoming);
        CBList(VertexID Node, EdgeID Edge, std::string path, ComputeMode mode);
        CBList(VertexID Node, EdgeID Edge, std::string path, EdgeID all_edge_num, std::vector<Gedge>& RemainEdgeList);
        bool InsertEdgeOut(VertexID src, AdjUnit NeighboorINFO);
        bool InsertEdgeIn(VertexID src, AdjUnit NeighboorINFO);
        bool InsertEdgeOutVLock(VertexID src, AdjUnit NeighboorINFO);
        bool InsertEdgeInVLock(VertexID src, AdjUnit NeighboorINFO);
        bool ReadEdgeOut(VertexID src, VertexID dst);
        bool ReadEdgeIn(VertexID src, VertexID dst);
        std::vector<VertexID> GetNeighborOut(VertexID src);
        std::vector<VertexID> GetNeighborIn(VertexID src);
        bool InsertGraphIE(std::string path, uint64_t startline, uint64_t endline);
        Gedge* GetInsertList(std::string path, uint64_t startline, uint64_t endline);
        void AddEdgeBatchOut(std::vector<Gedge_noWeight>& EdgeList, EdgeID edge_count);
        void AddEdgeBatchIn(std::vector<Gedge_noWeight>& EdgeList, EdgeID edge_count);
        void ConnectGTChain(nextPointer& GTChain, CBLNode*& VertexTable);
    };

    void Node::Print()
    {
        printf("this is a Node\n");
    }

    void InterNode::Print()
    {
        uint32_t i = 0, j = 0;
        for (uint32_t kk = 0;kk < INTER_NODE_KEY_CNT;++kk)
        {
            if (KeyList[kk] != -1) ++i;
            if (Child[kk] != nullptr) ++j;
        }
        if (Child[INTER_NODE_PTR_CNT - 1] != nullptr) ++j;
        printf("%d,%d,%d:", count, i, j);
        for (uint32_t i = 0;i < INTER_NODE_KEY_CNT;++i)
            printf("[%d]", KeyList[i]);
        printf("|||");
    }

    void LeafChunk::Print()
    {
        printf("%d:", count);
        for (uint32_t i = 0;i < LEAF_CHUNK_NCNT;++i)
            printf("(%d/%d)", NeighboorChunk[i].dest, NeighboorChunk[i].value);
        printf("==>");
    }

    bool LeafChunk::Insert(AdjUnit NewNeighboor)
    {
        uint32_t i = 0;
        for (; (NewNeighboor.dest > NeighboorChunk[i].dest) && (i < count); ++i)//按顺序 -- 优化二分？n~logn差不多
        {
        }
        for (uint32_t j = count; j > i; --j)//移动，找到应该插入的关键字位置
            NeighboorChunk[j] = NeighboorChunk[j - 1];
        NeighboorChunk[i] = NewNeighboor;//插入关键字
        ++count;
        return true;
    }

    bool InterNode::Insert(uint32_t dest, Node* NewNode)
    {
        uint32_t i = 0;
        for (; (i < count) && (dest > KeyList[i]); ++i)//i指向key要插入的位置
        {
        }
        for (uint32_t j = count; j > i; --j)//挪动倒地方
            KeyList[j] = KeyList[j - 1];
        for (uint32_t j = count + 1; j > i + 1; --j)//父亲key值改变，孩子移动；
            Child[j] = Child[j - 1];
        KeyList[i] = dest;//关键字传到父亲节点
        Child[i + 1] = NewNode;//newnode放到该放的位置
        NewNode->Parent = this;
        ++count;
        return true;
    }

    uint32_t LeafChunk::Split(LeafChunk* p)
    {
        assert(count == LEAF_CHUNK_NCNT);
        uint32_t j = 0;
        for (uint32_t i = LEAF_CHUNK_NCNT / 2; i < LEAF_CHUNK_NCNT; ++i, ++j)
        {
            p->NeighboorChunk[j] = this->NeighboorChunk[i];//this为old node
            this->NeighboorChunk[i] = { 0, 0 };
        }
        this->count = this->count - j;
        p->count = j;
        p->nextPtr = this->nextPtr;
        this->nextPtr.nextLeafChunk = p;
        p->nextType = this->nextType;
        this->nextType = LEAFCHUNK_LEVEL;
        p->Parent = this->Parent;

        return p->NeighboorChunk[0].dest;
    }

    int32_t InterNode::Split(InterNode* Brother, int32_t& NewKey)
    {
        assert(count == INTER_NODE_KEY_CNT);
        int32_t InsertPos = -1;
        InsertPos = NewKey > KeyList[count / 2] ? 1 : 0;
        NewKey = KeyList[count / 2];
        KeyList[count / 2] = -1;
        uint32_t i = count / 2 + 1, j = 0;
        for (;i < count;++i, ++j)
        {
            Brother->KeyList[j] = KeyList[i];
            Child[i]->Parent = Brother;
            Brother->Child[j] = Child[i];
            KeyList[i] = -1;
            Child[i] = nullptr;
        }
        Child[count]->Parent = Brother;
        Brother->Child[j] = Child[count];   //chunksize = 1或2 的时候有个问题
        Child[count] = nullptr;
        Brother->count = count - count / 2 - 1;
        this->count = count / 2;
        return InsertPos;// -1 失败 0 Newkey应在this中 1 应在brother中
    }

    bool BplusTree::Insert(AdjUnit NewNeighboor)
    {
        LeafChunk* InsertPos = Find(NewNeighboor.dest);
        if (InsertPos == nullptr)
        {
            InsertPos = new LeafChunk();
            Root = InsertPos;
            ++LeafCnt;
        }
        if (InsertPos->count < LEAF_CHUNK_NCNT)
            return InsertPos->Insert(NewNeighboor);
        LeafChunk* NewLeafChunk = new LeafChunk();
        uint32_t NewLeafChunkFirstDest = InsertPos->Split(NewLeafChunk);
        ++LeafCnt;
        if (NewNeighboor.dest < NewLeafChunkFirstDest)
        {
            InsertPos->Insert(NewNeighboor);
        }
        else
        {
            NewLeafChunk->Insert(NewNeighboor);
        }
        InterNode* this_parent = (InterNode*)(InsertPos->Parent);
        if (nullptr == this_parent)//初始化parent，若没有父结点，新建一个
        {
            InterNode* New_Root = new InterNode;
            New_Root->Child[0] = InsertPos;
            New_Root->KeyList[0] = NewLeafChunkFirstDest;
            New_Root->Child[1] = NewLeafChunk;
            InsertPos->Parent = New_Root;
            NewLeafChunk->Parent = New_Root;
            New_Root->count = 1;
            Root = New_Root;
            return true;
        }
        return Add_Node(this_parent, NewLeafChunkFirstDest, NewLeafChunk);
    }

    bool BplusTree::InsertKnowPos(AdjUnit NewNeighboor, LeafChunk* InsertPos)
    {
        if (InsertPos == nullptr)
        {
            InsertPos = new LeafChunk();
            Root = InsertPos;
            ++LeafCnt;
        }
        if (InsertPos->count < LEAF_CHUNK_NCNT)
            return InsertPos->Insert(NewNeighboor);
        LeafChunk* NewLeafChunk = new LeafChunk();
        uint32_t NewLeafChunkFirstDest = InsertPos->Split(NewLeafChunk);
        ++LeafCnt;
        if (NewNeighboor.dest < NewLeafChunkFirstDest)
        {
            InsertPos->Insert(NewNeighboor);
        }
        else
        {
            NewLeafChunk->Insert(NewNeighboor);
        }
        InterNode* this_parent = (InterNode*)(InsertPos->Parent);
        if (nullptr == this_parent)//初始化parent，若没有父结点，新建一个
        {
            InterNode* New_Root = new InterNode;
            New_Root->Child[0] = InsertPos;
            New_Root->KeyList[0] = NewLeafChunkFirstDest;
            New_Root->Child[1] = NewLeafChunk;
            InsertPos->Parent = New_Root;
            NewLeafChunk->Parent = New_Root;
            New_Root->count = 1;
            Root = New_Root;
            return true;
        }
        return Add_Node(this_parent, NewLeafChunkFirstDest, NewLeafChunk);
    }

    LeafChunk* BplusTree::Find(uint32_t dest)
    {
        uint32_t i = 0;
        Node* p = Root;
        InterNode* q;
        while (p != nullptr)
        {
            if (p->IsLeaf)
                break;
            for (i = 0; i < p->count; ++i)
            {
                if (dest < ((InterNode*)p)->KeyList[i])
                    break;
            }
            q = (InterNode*)p;
            p = q->Child[i];
        }

        return (LeafChunk*)p;//把根return,如果根为空,第一个插入函数生成的节点即为根
    }

    bool BplusTree::Add_Node(InterNode* Parent, VertexID NewKey, Node* NewNode)
    {
        if (nullptr == Parent || Parent->IsLeaf)
            return false;
        if (Parent->count < INTER_NODE_KEY_CNT)//父亲不满
            return Parent->Insert(NewKey, NewNode);
        InterNode* Brother = new InterNode;
        //叶子节点满，父节点也满分裂情况
        int32_t NewKey1 = NewKey;
        int32_t InsertPos = Parent->Split(Brother, NewKey1);//NewKey为需要提取并插入到root节点的值

        //确定需要插入的关键字，是插入到分裂节点的哪个位置
        if (InsertPos)
            Brother->Insert(NewKey, NewNode);
        else
            Parent->Insert(NewKey, NewNode);


        InterNode* Pparent = (InterNode*)(Parent->Parent);
        if (NULL == Pparent)
        {
            Pparent = new InterNode();
            Pparent->Child[0] = Parent;
            Pparent->KeyList[0] = NewKey1;//newkey为分裂传回，为插入的中间值
            Pparent->Child[1] = Brother;
            Parent->Parent = Pparent;
            Brother->Parent = Pparent;
            Pparent->count = 1;
            Root = Pparent;
            return true;
        }
        return Add_Node(Pparent, NewKey1, Brother);
    }

    void BplusTree::Print()//现在有点小问题 因为整体连起来了
    {
        // Node* p = Root;
        // if (NULL == p)
        //     return;
        // InterNode* a;
        // int H = 0;
        // queue<Node*> q;
        // queue<int> h;
        // q.push(p);
        // h.push(1);
        // while (!q.empty())
        // {
        //     p = q.front();
        //     if (H != h.front())
        //     {
        //         cout << endl;
        //         cout << H << endl;
        //         H = h.front();
        //     }
        //     q.pop();
        //     h.pop();
        //     p->Print();
        //     if (NULL != p && !p->IsLeaf)
        //     {
        //         a = (InterNode*)p;
        //         for (int i = 0; i <= p->count; ++i)
        //         {
        //             q.push(a->Child[i]);
        //             h.push(H + 1);
        //         }
        //     }
        // }
    }

    LeafChunk* BplusTree::GetLeafStart()
    {
        auto p = Root;
        if (p == nullptr) return nullptr;
        while (!p->IsLeaf)
        {
            p = ((InterNode*)p)->Child[0];
        }
        return (LeafChunk*)p;
    }

    LeafChunk* BplusTree::GetLeafEnd()
    {
        auto p = Root;
        if (p == nullptr) return nullptr;
        while (!p->IsLeaf)
        {
            p = ((InterNode*)p)->Child[p->count];
        }
        return (LeafChunk*)p;
    }
    CBList::CBList(std::string infopath, ComputeMode cm, bool load_graph_in_order)
    {
        if (cm != ComputeMode::Pull && cm != ComputeMode::Push && cm != ComputeMode::Mixed)
        {
            printf("mode error!\n");
            exit(-1);
        }

        auto [v, e, EdgeListFromFile] = LoadGraphProxy(infopath);
        this->writelockOut = std::vector<std::mutex>(v);
        this->writelockIn = std::vector<std::mutex>(v);
        this->vertex_lock = std::vector<std::mutex>(v);
        this->EdgeNum = e;
        this->VertexNum = v;
    #ifdef DEBUG
        std::cout << "file content size=" << EdgeListFromFile.size() << std::endl;
    #endif
        if (cm == ComputeMode::Pull)
        {
            this->VertexTableIn = new CBLNode[VertexNum];
            auto Lv1ClistIn = new Lv1Chunk[VertexNum];
            this->VertexTableOut = nullptr;
        #pragma omp parallel for num_threads(omp_get_max_threads()) 
            for (IndexType v_i = 0; v_i < VertexNum; ++v_i)
                this->VertexTableIn[v_i].Neighboor.nextLv1Chunk = Lv1ClistIn + v_i;
        #ifdef DEBUG
            std::cout << "load incomingCBList...\n";
        #endif
            if (load_graph_in_order)
            {
                for (auto& edge : EdgeListFromFile)
                    this->VertexTableIn[edge.end_vertex].Insert({ edge.start_vertex, edge.value });
            }
            else
            {
            #pragma omp parallel for num_threads(omp_get_max_threads())
                for (auto& edge : EdgeListFromFile)
                {
                    writelockIn[edge.end_vertex].lock();
                    this->VertexTableIn[edge.end_vertex].Insert({ edge.start_vertex, edge.value });
                    writelockIn[edge.end_vertex].unlock();
                }
            }
        }
        else if (cm == ComputeMode::Push)
        {
            this->VertexTableOut = new CBLNode[VertexNum];
            auto Lv1ClistOut = new Lv1Chunk[VertexNum];
            this->VertexTableIn = nullptr;
        #pragma omp parallel for num_threads(omp_get_max_threads()) 
            for (IndexType v_i = 0; v_i < VertexNum; ++v_i)
                this->VertexTableOut[v_i].Neighboor.nextLv1Chunk = Lv1ClistOut + v_i;
        #ifdef DEBUG
            std::cout << "load outgoingCBList...\n";
        #endif
            if (load_graph_in_order)
            {
                for (auto& edge : EdgeListFromFile)
                    this->VertexTableOut[edge.start_vertex].Insert({ edge.end_vertex, edge.value });
            }
            else
            {
            #pragma omp parallel for num_threads(omp_get_max_threads())
                for (auto& edge : EdgeListFromFile)
                {
                    writelockOut[edge.start_vertex].lock();
                    this->VertexTableOut[edge.start_vertex].Insert({ edge.end_vertex, edge.value });
                    writelockOut[edge.start_vertex].unlock();
                }
            }
        }
        else
        {
            this->VertexTableOut = new CBLNode[VertexNum];
            this->VertexTableIn = new CBLNode[VertexNum];
            Lv1Chunk* Lv1ClistOut = new Lv1Chunk[VertexNum];
            Lv1Chunk* Lv1ClistIn = new Lv1Chunk[VertexNum];

        #pragma omp parallel for num_threads(omp_get_max_threads()) 
            for (IndexType v_i = 0; v_i < VertexNum; ++v_i)
            {
                this->VertexTableOut[v_i].Neighboor.nextLv1Chunk = Lv1ClistOut + v_i;
                this->VertexTableIn[v_i].Neighboor.nextLv1Chunk = Lv1ClistIn + v_i;
            }
            if (load_graph_in_order)
            {
                for (auto& edge : EdgeListFromFile)
                {
                    this->VertexTableOut[edge.start_vertex].Insert({ edge.end_vertex, edge.value });
                    this->VertexTableIn[edge.end_vertex].Insert({ edge.start_vertex, edge.value });
                }
            }
            else
            {
            #pragma omp parallel for num_threads(omp_get_max_threads()) 
                for (auto& edge : EdgeListFromFile)
                {
                    std::scoped_lock double_lock(writelockOut[edge.start_vertex], writelockIn[edge.end_vertex]);
                    this->VertexTableOut[edge.start_vertex].Insert({ edge.end_vertex, edge.value });
                    this->VertexTableIn[edge.end_vertex].Insert({ edge.start_vertex, edge.value });
                }
            }
        }

        //assert(max_ == VertexNum - 1);
        //assert(EdgeNum == edge_cnts); //可能文件中有首行说明
        if (cm == ComputeMode::Pull || cm == ComputeMode::Mixed)
            ConnectGTChain(this->GTChainIn, this->VertexTableIn);
        if (cm == ComputeMode::Push || cm == ComputeMode::Mixed)
            ConnectGTChain(this->GTChainOut, this->VertexTableOut);
    }
    //TODO:unsorted ---- insert method
    CBList::CBList(VertexID VertexNum, EdgeID EdgeNum, std::string path) :writelockOut(VertexNum), writelockIn(VertexNum), vertex_lock(VertexNum)
    {
        this->EdgeNum = EdgeNum;
        this->VertexNum = VertexNum;
        this->VertexTableOut = new CBLNode[VertexNum];
        Lv1Chunk* Lv1Clist = new Lv1Chunk[VertexNum];
    #pragma omp parallel for num_threads(omp_get_max_threads()) 
        for (int i = 0;i < VertexNum;++i)
            this->VertexTableOut[i].Neighboor.nextLv1Chunk = Lv1Clist + i;
        auto EdgeListFromFile = LoadGraphFromBGBinaryFile(EdgeNum, path);
        std::cout << "file_content_size=" << EdgeListFromFile.size() << std::endl;
        std::cout << "load outgoingCBList...\n";
        for (auto& edge : EdgeListFromFile)
            this->VertexTableOut[edge.start_vertex].Insert({ edge.end_vertex, edge.value });
        //assert(max_ == VertexNum - 1);
        //assert(EdgeNum == edge_cnts); //可能文件中有首行说明
        this->GTChainOut = VertexTableOut[0].Neighboor;
    #pragma omp parallel for num_threads(omp_get_max_threads()) 
        for (VertexID i = 1;i < VertexNum;++i)
        {
            if (VertexTableOut[i - 1].Level == 1)
            {
                VertexTableOut[i - 1].Neighboor.nextLv1Chunk->nextPtr = VertexTableOut[i].Neighboor;
                if (VertexTableOut[i].NeighboorCnt <= LV1CHUNK_NCNT)
                    VertexTableOut[i - 1].Neighboor.nextLv1Chunk->nextType = 0 - CHUNK_LEVEL;
                else VertexTableOut[i - 1].Neighboor.nextLv1Chunk->nextType = 0 - LEAFCHUNK_LEVEL;
            }
            else if (VertexTableOut[i - 1].Level == 2)
            {
                VertexTableOut[i - 1].BPT.GetLeafEnd()->nextPtr = VertexTableOut[i].Neighboor;
                if (VertexTableOut[i].NeighboorCnt <= LV1CHUNK_NCNT)
                    VertexTableOut[i - 1].BPT.GetLeafEnd()->nextType = 0 - CHUNK_LEVEL;
                else VertexTableOut[i - 1].BPT.GetLeafEnd()->nextType = 0 - LEAFCHUNK_LEVEL;
            }
            else
            {
                printf("load graph error-Level?\n");
                exit(-1);
            }
        }
        if (VertexTableOut[VertexNum - 1].Level == 1)
            VertexTableOut[VertexNum - 1].Neighboor.nextLv1Chunk->nextType = -10;
        else if (VertexTableOut[VertexNum - 1].Level == 2)
            VertexTableOut[VertexNum - 1].Neighboor.nextLeafChunk->nextType = -10;
    }

    CBList::CBList(VertexID VertexNum, EdgeID EdgeNum, std::string path, bool incoming) :writelockOut(VertexNum), writelockIn(VertexNum), vertex_lock(VertexNum)
    {
        this->EdgeNum = EdgeNum;
        this->VertexNum = VertexNum;
        this->VertexTableIn = new CBLNode[VertexNum];
        Lv1Chunk* Lv1Clist = new Lv1Chunk[VertexNum];
    #pragma omp parallel for num_threads(omp_get_max_threads()) 
        for (int i = 0;i < VertexNum;++i)
            this->VertexTableIn[i].Neighboor.nextLv1Chunk = Lv1Clist + i;
        auto EdgeListFromFile = LoadGraphFromBGBinaryFile(EdgeNum, path);
        std::cout << "file_content_size=" << EdgeListFromFile.size() << std::endl;
        std::cout << "load incomingCBList...\n";

        for (auto& edge : EdgeListFromFile)
            this->VertexTableIn[edge.end_vertex].Insert({ edge.start_vertex, edge.value });

        //assert(max_ == VertexNum - 1);
        //assert(EdgeNum == edge_cnts); //可能文件中有首行说明
        this->GTChainIn = VertexTableIn[0].Neighboor;
    #pragma omp parallel for num_threads(omp_get_max_threads()) 
        for (VertexID i = 1;i < VertexNum;++i)
        {
            if (VertexTableIn[i - 1].Level == 1)
            {
                VertexTableIn[i - 1].Neighboor.nextLv1Chunk->nextPtr = VertexTableIn[i].Neighboor;
                if (VertexTableIn[i].NeighboorCnt <= LV1CHUNK_NCNT)
                    VertexTableIn[i - 1].Neighboor.nextLv1Chunk->nextType = 0 - CHUNK_LEVEL;
                else VertexTableIn[i - 1].Neighboor.nextLv1Chunk->nextType = 0 - LEAFCHUNK_LEVEL;
            }
            else if (VertexTableIn[i - 1].Level == 2)
            {
                VertexTableIn[i - 1].BPT.GetLeafEnd()->nextPtr = VertexTableIn[i].Neighboor;
                if (VertexTableIn[i].NeighboorCnt <= LV1CHUNK_NCNT)
                    VertexTableIn[i - 1].BPT.GetLeafEnd()->nextType = 0 - CHUNK_LEVEL;
                else VertexTableIn[i - 1].BPT.GetLeafEnd()->nextType = 0 - LEAFCHUNK_LEVEL;
            }
            else
            {
                printf("load graph error-Level?\n");
                exit(-1);
            }
        }
        if (VertexTableIn[VertexNum - 1].Level == 1)
            VertexTableIn[VertexNum - 1].Neighboor.nextLv1Chunk->nextType = -10;
        else if (VertexTableIn[VertexNum - 1].Level == 2)
            VertexTableIn[VertexNum - 1].Neighboor.nextLeafChunk->nextType = -10;
    }

    CBList::CBList(VertexID VertexNum, EdgeID EdgeNum, std::string path, ComputeMode mode) :writelockOut(VertexNum), writelockIn(VertexNum), vertex_lock(VertexNum)
    {
        printf("Abandon this CTOR for now.\n");
        // if(mode >= 1 && mode <= 2)
        // {
        //     printf("Weight = 1 mode!\n");
        //     this->EdgeNum = EdgeNum;
        //     this->VertexNum = VertexNum;
        //     this->VertexTable = new CBLNode[VertexNum];
        //     Lv1Chunk* Lv1Clist = new Lv1Chunk[VertexNum];
        //     #pragma omp parallel for num_threads(omp_get_max_threads()) 
        //     for(int i=0;i<VertexNum;++i)
        //         this->VertexTable[i].Neighboor.nextLv1Chunk = Lv1Clist+i;
        //     auto EdgeListFromFile = LoadGraphFromBGBinaryFile(EdgeNum, path);
        //     std::cout << "file_content_size="<< EdgeListFromFile.size() << std::endl;

        //     if(mode == 1)
        //     {
        //         std::cout << "load incomingCBList...\n";
        //         for(auto& edge:EdgeListFromFile)
        //             this->VertexTable[edge.end_vertex].Insert({edge.start_vertex, 1});
        //     }
        //     else if(mode == 2)
        //     {
        //         std::cout << "load outgoingCBList...\n";
        //         for(auto& edge:EdgeListFromFile)
        //             this->VertexTable[edge.start_vertex].Insert({edge.end_vertex, 1});
        //     }

        //     //assert(max_ == VertexNum - 1);
        //     //assert(EdgeNum == edge_cnts); //可能文件中有首行说明
        //     this->GTChain = VertexTable[0].Neighboor; 
        //     #pragma omp parallel for num_threads(omp_get_max_threads()) 
        //     for(VertexID i=1;i<VertexNum;++i)
        //     {
        //         if(VertexTable[i-1].Level == 1)
        //         {             
        //             VertexTable[i-1].Neighboor.nextLv1Chunk->nextPtr = VertexTable[i].Neighboor;
        //             if(VertexTable[i].NeighboorCnt <= LV1CHUNK_NCNT) 
        //                 VertexTable[i-1].Neighboor.nextLv1Chunk->nextType = 0 - CHUNK_LEVEL;
        //             else VertexTable[i-1].Neighboor.nextLv1Chunk->nextType = 0 - LEAFCHUNK_LEVEL;
        //         }
        //         else if(VertexTable[i-1].Level == 2)
        //         {
        //             VertexTable[i-1].BPT.GetLeafEnd()->nextPtr = VertexTable[i].Neighboor;
        //             if(VertexTable[i].NeighboorCnt <= LV1CHUNK_NCNT) 
        //                 VertexTable[i-1].BPT.GetLeafEnd()->nextType = 0 - CHUNK_LEVEL;
        //             else VertexTable[i-1].BPT.GetLeafEnd()->nextType = 0 - LEAFCHUNK_LEVEL;
        //         }
        //         else
        //         {
        //             printf("load graph error-Level?\n");
        //             exit(-1);
        //         }
        //     }
        //     if(VertexTable[VertexNum-1].Level==1)
        //         VertexTable[VertexNum-1].Neighboor.nextLv1Chunk->nextType = -10;
        //     else if(VertexTable[VertexNum-1].Level==2)
        //         VertexTable[VertexNum-1].Neighboor.nextLeafChunk->nextType = -10;
        // }
        // else
        // {
        //     printf("mode error!\n");
        //     exit(-1);
        // }
    }

    CBList::CBList(VertexID VertexNum, EdgeID EdgeNum, std::string path, EdgeID all_edge_num, std::vector<Gedge>& RemainEdgeList) :
        writelockOut(VertexNum), writelockIn(VertexNum), vertex_lock(VertexNum)
    {
        printf("Abandon this CTOR for now.\n");
        // this->EdgeNum = EdgeNum;
        // this->VertexNum = VertexNum;
        // this->VertexTable = new CBLNode[VertexNum];
        // Lv1Chunk* Lv1Clist = new Lv1Chunk[VertexNum];
        // #pragma omp parallel for num_threads(omp_get_max_threads()) 
        // for(int i=0;i<VertexNum;++i)
        //     this->VertexTable[i].Neighboor.nextLv1Chunk = Lv1Clist+i;
        // auto EdgeListFromFile = LoadGraphFromBGBinaryFile(all_edge_num, path);
        // std::cout << "file_content_size="<< EdgeListFromFile.size() << std::endl;
        // for(EdgeID i=0;i<EdgeNum;++i)
        //     this->VertexTable[EdgeListFromFile[i].start_vertex].Insert({EdgeListFromFile[i].end_vertex, EdgeListFromFile[i].value});
        // EdgeListFromFile.assign(EdgeListFromFile.begin()+EdgeNum, EdgeListFromFile.end());
        // RemainEdgeList.swap(EdgeListFromFile);
        // //assert(max_ == VertexNum - 1);
        // //assert(EdgeNum == edge_cnts); //可能文件中有首行说明
        // this->GTChain = VertexTable[0].Neighboor; 
        // for(VertexID i=1;i<VertexNum;++i)
        // {
        //     if(VertexTable[i-1].Level == 1)
        //     {             
        //         VertexTable[i-1].Neighboor.nextLv1Chunk->nextPtr = VertexTable[i].Neighboor;
        //         if(VertexTable[i].NeighboorCnt <= LV1CHUNK_NCNT) 
        //             VertexTable[i-1].Neighboor.nextLv1Chunk->nextType = 0 - CHUNK_LEVEL;
        //         else VertexTable[i-1].Neighboor.nextLv1Chunk->nextType = 0 - LEAFCHUNK_LEVEL;
        //     }
        //     else if(VertexTable[i-1].Level == 2)
        //     {
        //         VertexTable[i-1].BPT.GetLeafEnd()->nextPtr = VertexTable[i].Neighboor;
        //         if(VertexTable[i].NeighboorCnt <= LV1CHUNK_NCNT) 
        //             VertexTable[i-1].BPT.GetLeafEnd()->nextType = 0 - CHUNK_LEVEL;
        //         else VertexTable[i-1].BPT.GetLeafEnd()->nextType = 0 - LEAFCHUNK_LEVEL;
        //     }
        //     else
        //     {
        //         printf("load graph error-Level?\n");
        //         exit(-1);
        //     }
        // }
        // if(VertexTable[VertexNum-1].Level==1)
        //     VertexTable[VertexNum-1].Neighboor.nextLv1Chunk->nextType = -10;
        // else if(VertexTable[VertexNum-1].Level==2)
        //     VertexTable[VertexNum-1].Neighboor.nextLeafChunk->nextType = -10;
    }

    CBLNode::CBLNode()
    {
        auto ptr_tmp = new BplusTree;
        this->BPT = *ptr_tmp;
        this->ChunkCnt = 1;
        this->Level = 1;
        this->NeighboorCnt = 0;
        this->Neighboor.nextLv1Chunk = nullptr;
    }

    [[nodiscard("InsertState")]] uint32_t CBLNode::Insert(AdjUnit NeighboorINFO)
    {
        if (this->NeighboorCnt < LV1CHUNK_NCNT)
        {
            this->Neighboor.nextLv1Chunk->NeighboorChunk[this->Neighboor.nextLv1Chunk->count] = NeighboorINFO;
            ++this->Neighboor.nextLv1Chunk->count;
            ++this->NeighboorCnt;
            return 1;
        }
        else if (this->NeighboorCnt == LV1CHUNK_NCNT)
        {
            ++this->Level;
            for (uint32_t i = 0;i < NeighboorCnt;++i)
            {
                BPT.Insert(this->Neighboor.nextLv1Chunk->NeighboorChunk[i]);
            }
            BPT.Insert(NeighboorINFO);
            ++this->NeighboorCnt;
            auto LastLeaf = BPT.GetLeafEnd();
            LastLeaf->nextType = this->Neighboor.nextLv1Chunk->nextType;
            LastLeaf->nextPtr = this->Neighboor.nextLv1Chunk->nextPtr;
            // free(this->Neighboor.nextLv1Chunk);
            this->Neighboor.nextLeafChunk = BPT.GetLeafStart();
            this->ChunkCnt = BPT.LeafCnt;
            return 2;
        }
        else
        {
            ++this->NeighboorCnt;
            BPT.Insert(NeighboorINFO);
            this->ChunkCnt = BPT.LeafCnt;
            return 1;
        }
        return 0;
    }

    bool CBList::InsertEdgeOut(VertexID src, AdjUnit NeighboorINFO)
    {
        uint32_t flag = this->VertexTableOut[src].Insert(NeighboorINFO);
        if (flag == 0) return false;
        if (flag == 2 && src != 0)
        {
            uint32_t level = this->VertexTableOut[src - 1].Level;
            if (level == 1)
            {
                this->VertexTableOut[src - 1].Neighboor.nextLv1Chunk->nextPtr = this->VertexTableOut[src].Neighboor;
                this->VertexTableOut[src - 1].Neighboor.nextLv1Chunk->nextType = LEAFCHUNK_LEVEL;
            }
            else if (level == 2)
            {
                this->VertexTableOut[src - 1].BPT.GetLeafEnd()->nextPtr = this->VertexTableOut[src].Neighboor;
                this->VertexTableOut[src - 1].BPT.GetLeafEnd()->nextType = LEAFCHUNK_LEVEL;
            }
        }
        ++this->EdgeNum;
        return true;
    }

    bool CBList::InsertEdgeIn(VertexID src, AdjUnit NeighboorINFO)
    {
        uint32_t flag = this->VertexTableIn[src].Insert(NeighboorINFO);
        if (flag == 0) return false;
        if (flag == 2 && src != 0)
        {
            uint32_t level = this->VertexTableIn[src - 1].Level;
            if (level == 1)
            {
                this->VertexTableIn[src - 1].Neighboor.nextLv1Chunk->nextPtr = this->VertexTableIn[src].Neighboor;
                this->VertexTableIn[src - 1].Neighboor.nextLv1Chunk->nextType = LEAFCHUNK_LEVEL;
            }
            else if (level == 2)
            {
                this->VertexTableIn[src - 1].BPT.GetLeafEnd()->nextPtr = this->VertexTableIn[src].Neighboor;
                this->VertexTableIn[src - 1].BPT.GetLeafEnd()->nextType = LEAFCHUNK_LEVEL;
            }
        }
        ++this->EdgeNum;
        return true;
    }

    // TODO: GTChain lock problem
    bool CBList::InsertEdgeOutVLock(VertexID src, AdjUnit NeighboorINFO)
    {
        writelockOut[src].lock();
        uint32_t flag = this->VertexTableOut[src].Insert(NeighboorINFO);

        if (flag == 0)
        {
            writelockOut[src].unlock();
            return false;
        }
        // if(flag == 2 && src != 0)
        // {
        //     uint32_t level = this->NodeList[src-1].Level;
        //     if(level == 1)
        //     {
        //         this->NodeList[src-1].Neighboor.nextLv1Chunk->nextPtr = this->NodeList[src].Neighboor;
        //         this->NodeList[src-1].Neighboor.nextLv1Chunk->nextType = LEAFCHUNK_LEVEL;
        //     }
        //     else if(level == 2)
        //     {
        //         this->NodeList[src-1].BPT.GetLeafEnd()->nextPtr = this->NodeList[src].Neighboor;
        //         this->NodeList[src-1].BPT.GetLeafEnd()->nextType = LEAFCHUNK_LEVEL;
        //     }
        // }
        ++this->EdgeNum;
        writelockOut[src].unlock();
        return true;
    }

    // TODO: GTChain lock problem
    bool CBList::InsertEdgeInVLock(VertexID src, AdjUnit NeighboorINFO)
    {
        writelockIn[src].lock();
        uint32_t flag = this->VertexTableIn[src].Insert(NeighboorINFO);

        if (flag == 0)
        {
            writelockIn[src].unlock();
            return false;
        }
        // if(flag == 2 && src != 0)
        // {
        //     uint32_t level = this->NodeList[src-1].Level;
        //     if(level == 1)
        //     {
        //         this->NodeList[src-1].Neighboor.nextLv1Chunk->nextPtr = this->NodeList[src].Neighboor;
        //         this->NodeList[src-1].Neighboor.nextLv1Chunk->nextType = LEAFCHUNK_LEVEL;
        //     }
        //     else if(level == 2)
        //     {
        //         this->NodeList[src-1].BPT.GetLeafEnd()->nextPtr = this->NodeList[src].Neighboor;
        //         this->NodeList[src-1].BPT.GetLeafEnd()->nextType = LEAFCHUNK_LEVEL;
        //     }
        // }
        ++this->EdgeNum;
        writelockIn[src].unlock();
        return true;
    }

    //Inserts are Edges
    bool CBList::InsertGraphIE(std::string path, uint64_t startline, uint64_t endline)
    {
        printf("Abandon this CTOR for now.\n");
        // auto EdgeListFromFile = LoadGraphFromFilePart(path, startline, endline);
        // //==============================================================time_start====================
        // auto start = std::chrono::steady_clock::now();

        // for(auto& edge:EdgeListFromFile)
        //     this->InsertEdge(edge.start_vertex, {edge.end_vertex, random(10000)});

        // auto end = std::chrono::steady_clock::now();
        // //==============================================================================time_stop===========================
        // std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
        // printf("Insert time: %fus\n",elapsed.count());

        // if(VertexTable[VertexNum-1].Level==1)
        //     VertexTable[VertexNum-1].Neighboor.nextLv1Chunk->nextType = -10;
        // else if(VertexTable[VertexNum-1].Level==2)
        //     VertexTable[VertexNum-1].Neighboor.nextLeafChunk->nextType = -10;
        // return true;
    }

    //TODO: judge mode (scan or b+tree) / one LeafNode BinarySearch
    bool CBList::ReadEdgeOut(VertexID src, VertexID dst)
    {
        if (this->VertexTableOut[src].Level == 1)
        {
            for (IndexType Nindex = 0; Nindex < VertexTableOut[src].Neighboor.nextLv1Chunk->count; ++Nindex)
            {
                if (VertexTableOut[src].Neighboor.nextLv1Chunk->NeighboorChunk[Nindex].dest == dst)
                    return true;
            }
        }
        else
        {
            auto TargetLeaf = this->VertexTableOut[src].BPT.Find(dst);
            if (TargetLeaf != nullptr)
            {
                for (IndexType Nindex = 0; Nindex < TargetLeaf->count; ++Nindex)
                {
                    if (TargetLeaf->NeighboorChunk[Nindex].dest == dst)
                        return true;
                }
            }
        }
        return false;
    }

    bool CBList::ReadEdgeIn(VertexID src, VertexID dst)
    {
        if (this->VertexTableIn[src].Level == 1)
        {
            for (IndexType Nindex = 0; Nindex < VertexTableIn[src].Neighboor.nextLv1Chunk->count; ++Nindex)
            {
                if (VertexTableIn[src].Neighboor.nextLv1Chunk->NeighboorChunk[Nindex].dest == dst)
                    return true;
            }
        }
        else
        {
            auto TargetLeaf = this->VertexTableIn[src].BPT.Find(dst);
            if (TargetLeaf != nullptr)
            {
                for (IndexType Nindex = 0; Nindex < TargetLeaf->count; ++Nindex)
                {
                    if (TargetLeaf->NeighboorChunk[Nindex].dest == dst)
                        return true;
                }
            }
        }
        return false;
    }

    std::vector<VertexID> CBList::GetNeighborOut(VertexID src)
    {
        std::vector<VertexID> NeighboorList(this->VertexTableOut[src].NeighboorCnt, 0);
        IndexType NLindex = 0;
        if (this->VertexTableOut[src].Level == 1)
        {
            for (IndexType Nindex = 0; Nindex < VertexTableOut[src].Neighboor.nextLv1Chunk->count; ++Nindex)
            {
                VertexID dst1 = VertexTableOut[src].Neighboor.nextLv1Chunk->NeighboorChunk[Nindex].dest;
                NeighboorList[NLindex++] = dst1;
            }
        }
        else
        {
            auto n_ptr = this->VertexTableOut[src].Neighboor.nextLeafChunk;
            for (IndexType chunkcnt = 0; chunkcnt < this->VertexTableOut[src].ChunkCnt; ++chunkcnt)
            {
                for (IndexType Nindex = 0; Nindex < n_ptr->count; ++Nindex)
                {
                    VertexID dst1 = n_ptr->NeighboorChunk[Nindex].dest;
                    NeighboorList[NLindex++] = dst1;
                }
                n_ptr = n_ptr->nextPtr.nextLeafChunk;
            }
        }
        return NeighboorList;
    }

    std::vector<VertexID> CBList::GetNeighborIn(VertexID src)
    {
        std::vector<VertexID> NeighboorList(this->VertexTableIn[src].NeighboorCnt, 0);
        IndexType NLindex = 0;
        if (this->VertexTableIn[src].Level == 1)
        {
            for (IndexType Nindex = 0; Nindex < VertexTableIn[src].Neighboor.nextLv1Chunk->count; ++Nindex)
            {
                VertexID dst1 = VertexTableIn[src].Neighboor.nextLv1Chunk->NeighboorChunk[Nindex].dest;
                NeighboorList[NLindex++] = dst1;
            }
        }
        else
        {
            auto n_ptr = this->VertexTableIn[src].Neighboor.nextLeafChunk;
            for (IndexType chunkcnt = 0; chunkcnt < this->VertexTableIn[src].ChunkCnt; ++chunkcnt)
            {
                for (IndexType Nindex = 0; Nindex < n_ptr->count; ++Nindex)
                {
                    VertexID dst1 = n_ptr->NeighboorChunk[Nindex].dest;
                    NeighboorList[NLindex++] = dst1;
                }
                n_ptr = n_ptr->nextPtr.nextLeafChunk;
            }
        }
        return NeighboorList;
    }

    void CBList::AddEdgeBatchOut(std::vector<Gedge_noWeight>& EdgeList, EdgeID edge_count)
    {
        // generate partitions array
        std::vector<uint32_t> parts;
        VertexID cur_src = EdgeList[0].start_vertex;
        parts.emplace_back(0);
        for (IndexType i = 1; i < edge_count; i++) {
            if (cur_src != EdgeList[i].start_vertex) {
                parts.emplace_back(i);
                cur_src = EdgeList[i].start_vertex;
            }
        }
        parts.emplace_back(edge_count);
        //TODO: 总体的threads参数。
        omp_set_num_threads(32);
    #pragma omp parallel for
        for (IndexType i = 0; i < parts.size() - 1; ++i)
        {
            //TODO: 遍历链在并行时候的问题。
            //TODO: 有batch 考虑块？
            //TODO: 无权图的时候 改一下现在这里的有权图的插入。
            for (IndexType ei = parts[i]; ei < parts[i + 1]; ++ei)
            {
                this->VertexTableOut[EdgeList[ei].start_vertex].Insert(AdjUnit({ EdgeList[ei].end_vertex, 0 }));
                // this->InsertEdge();
            }
        }
    }


    void CBList::AddEdgeBatchIn(std::vector<Gedge_noWeight>& EdgeList, EdgeID edge_count)
    {
        // generate partitions array
        std::vector<uint32_t> parts;
        VertexID cur_src = EdgeList[0].start_vertex;
        parts.emplace_back(0);
        for (IndexType i = 1; i < edge_count; i++) {
            if (cur_src != EdgeList[i].start_vertex) {
                parts.emplace_back(i);
                cur_src = EdgeList[i].start_vertex;
            }
        }
        parts.emplace_back(edge_count);
        //TODO: 总体的threads参数。
        omp_set_num_threads(32);
    #pragma omp parallel for
        for (IndexType i = 0; i < parts.size() - 1; ++i)
        {
            //TODO: 遍历链在并行时候的问题。
            //TODO: 有batch 考虑块？
            //TODO: 无权图的时候 改一下现在这里的有权图的插入。
            for (IndexType ei = parts[i]; ei < parts[i + 1]; ++ei)
            {
                this->VertexTableIn[EdgeList[ei].end_vertex].Insert(AdjUnit({ EdgeList[ei].start_vertex, 0 }));
                // this->InsertEdge();
            }
        }
    }


    //========================================TODO: Just support outgoing for now.============================
    struct MetaInfo
    {
        int bt_num;
        int lv1_num;
        std::vector<VertexID> bt_record;
        CBList& cbl;
        MetaInfo(CBList& graph) :bt_num(0), lv1_num(0), bt_record(0), cbl(graph) {};
        void getInfo()
        {
            for (IndexType v = 0; v < this->cbl.VertexNum; ++v)
            {
                if (this->cbl.VertexTableOut[v].NeighboorCnt > LV1CHUNK_NCNT)
                {
                    ++bt_num;
                    bt_record.emplace_back(v);
                }
                else
                    ++lv1_num;
            }
            std::cout << bt_num << "  " << lv1_num << std::endl;
            std::cout << (double)(bt_num) / this->cbl.VertexNum << "-" << (double)(lv1_num) / this->cbl.VertexNum << std::endl;
        }
    };

    CBList::EdgeIterator::EdgeIterator(const CBList* Graph, VertexID src) :v(src), CurrentPos(0), AccessEdgeCnt(0), alive(true)
    {
        level = Graph->VertexTableOut[src].Level;
        NeighboorCnt = Graph->VertexTableOut[src].NeighboorCnt;
        if (level == 1) CurrentPtr.nextLv1Chunk = Graph->VertexTableOut[src].Neighboor.nextLv1Chunk;
        else CurrentPtr.nextLeafChunk = Graph->VertexTableOut[src].Neighboor.nextLeafChunk;
    }

    CBList::EdgeIterator CBList::EdgeIterator::operator++()
    {
        ++CurrentPos;
        ++AccessEdgeCnt;
        if (AccessEdgeCnt >= NeighboorCnt)
        {
            alive = false;
            return *this;
        }
        else if (level == 2)
        {
            if (CurrentPos >= CurrentPtr.nextLeafChunk->count)
            {
                CurrentPos = 0;
                CurrentPtr = CurrentPtr.nextLeafChunk->nextPtr;
            }
        }
        return *this;
    }

    AdjUnit CBList::EdgeIterator::GetEdegInfo()
    {
        if (alive)
        {
            if (level == 1)
                return CurrentPtr.nextLv1Chunk->NeighboorChunk[CurrentPos];
            else
                return CurrentPtr.nextLeafChunk->NeighboorChunk[CurrentPos];
        }
        else
        {
            std::cout << "Unauthorized access in EdgeIterator(" << v << ")\n";
            exit(-1);
        }
    }

    void CBList::ConnectGTChain(nextPointer& GTChain, CBLNode*& VertexTable)
    {
        GTChain = VertexTable[0].Neighboor;
    #pragma omp parallel for num_threads(omp_get_max_threads()) 
        for (VertexID i = 1;i < VertexNum;++i)
        {
            if (VertexTable[i - 1].Level == 1)
            {
                VertexTable[i - 1].Neighboor.nextLv1Chunk->nextPtr = VertexTable[i].Neighboor;
                if (VertexTable[i].NeighboorCnt <= LV1CHUNK_NCNT)
                    VertexTable[i - 1].Neighboor.nextLv1Chunk->nextType = 0 - CHUNK_LEVEL;
                else VertexTable[i - 1].Neighboor.nextLv1Chunk->nextType = 0 - LEAFCHUNK_LEVEL;
            }
            else if (VertexTable[i - 1].Level == 2)
            {
                VertexTable[i - 1].BPT.GetLeafEnd()->nextPtr = VertexTable[i].Neighboor;
                if (VertexTable[i].NeighboorCnt <= LV1CHUNK_NCNT)
                    VertexTable[i - 1].BPT.GetLeafEnd()->nextType = 0 - CHUNK_LEVEL;
                else VertexTable[i - 1].BPT.GetLeafEnd()->nextType = 0 - LEAFCHUNK_LEVEL;
            }
            else
            {
                printf("load graph error-Level?\n");
                exit(-1);
            }
        }
        if (VertexTable[VertexNum - 1].Level == 1)
            VertexTable[VertexNum - 1].Neighboor.nextLv1Chunk->nextType = -10;
        else if (VertexTable[VertexNum - 1].Level == 2)
            VertexTable[VertexNum - 1].Neighboor.nextLeafChunk->nextType = -10;
    }

}//namespace GastCoCo