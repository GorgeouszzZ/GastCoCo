
#include <functional>
#include <omp.h>

#include "../other/type.hpp"
#include "../CBList/CBList.hpp"
#include "bitmap.hpp"
#include "../workload/Slice_for_CBL_coro.hpp"
#include "../other/Coro_graph_v2.hpp"




template <typename EdgeData = Empty>
class Graph {
public:
    CoroGraph::CBList* cblptr;
    CoroGraph::CBList* cblptroutgoing;
    std::vector<uint32_t> SFEresult;
    std::vector<uint32_t> SFEresultoutgoing;
    std::vector<int> SFVresult;
//   int partition_id;
//   int partitions;

//   size_t alpha;

  int threads;
  int coroutines;
//   int sockets;
//   int threads_per_socket;

//   size_t edge_data_size;
//   size_t unit_size;
//   size_t edge_unit_size;

//   bool symmetric;
//   VertexID vertices;
//   EdgeID edges;
//   VertexID * out_degree; // VertexId [vertices]; numa-aware
//   VertexID * in_degree; // VertexId [vertices]; numa-aware

//   VertexID * partition_offset; // VertexId [partitions+1]
//   VertexID * local_partition_offset; // VertexId [sockets+1]

//   VertexID owned_vertices;
//   EdgeID * outgoing_edges; // EdgeId [sockets]
//   EdgeID * incoming_edges; // EdgeId [sockets]

//   Bitmap ** incoming_adj_bitmap;
//   EdgeID ** incoming_adj_index; // EdgeId [sockets] [vertices+1]; numa-aware
//   AdjUnit<EdgeData> ** incoming_adj_list; // AdjUnit<EdgeData> [sockets] [vertices+1]; numa-aware
//   Bitmap ** outgoing_adj_bitmap;
//   EdgeID ** outgoing_adj_index; // EdgeId [sockets] [vertices+1]; numa-aware
//   AdjUnit<EdgeData> ** outgoing_adj_list; // AdjUnit<EdgeData> [sockets] [vertices+1]; numa-aware

//   VertexID * compressed_incoming_adj_vertices;
//   CompressedAdjIndexUnit ** compressed_incoming_adj_index; // CompressedAdjIndexUnit [sockets] [...+1]; numa-aware
//   VertexID * compressed_outgoing_adj_vertices;
//   CompressedAdjIndexUnit ** compressed_outgoing_adj_index; // CompressedAdjIndexUnit [sockets] [...+1]; numa-aware

  ThreadState ** thread_state; // ThreadState* [threads]; numa-aware

  ThreadState ** thread_state_outgoing;
  ThreadState ** tuned_chunks_dense; // ThreadState [partitions][threads];
  ThreadState ** tuned_chunks_sparse; // ThreadState [partitions][threads];

//   size_t local_send_buffer_limit;
//   MessageBuffer ** local_send_buffer; // MessageBuffer* [threads]; numa-aware

//   int current_send_part_id;
//   MessageBuffer *** send_buffer; // MessageBuffer* [partitions] [sockets]; numa-aware
//   MessageBuffer *** recv_buffer; // MessageBuffer* [partitions] [sockets]; numa-aware

  Graph() {
    // threads = numa_num_configured_cpus();
    // sockets = numa_num_configured_nodes();
    // threads_per_socket = threads / sockets;
    threads = 52;
    coroutines = 2;

    cblptr = nullptr;
    cblptroutgoing = nullptr;

    init();
  }

//   inline int get_socket_id(int thread_id) {
//     return thread_id / threads_per_socket;
//   }

//   inline int get_socket_offset(int thread_id) {
//     return thread_id % threads_per_socket;
//   }

  void init() {
    // edge_data_size = std::is_same<EdgeData, Empty>::value ? 0 : sizeof(EdgeData);
    // unit_size = sizeof(VertexId) + edge_data_size;
    // edge_unit_size = sizeof(VertexId) + unit_size;

    // assert( numa_available() != -1 );
    // assert( sizeof(unsigned long) == 8 ); // assume unsigned long is 64-bit

    // char nodestring[sockets*2+1];
    // nodestring[0] = '\0';
    // for (int s_i=1;s_i<sockets;s_i++) {
    //   nodestring[s_i*2-1] = ',';
    //   nodestring[s_i*2] = '0'+s_i;
    // }
    // struct bitmask * nodemask = numa_parse_nodestring(nodestring);
    // numa_set_interleave_mask(nodemask);

    omp_set_dynamic(0);
    omp_set_num_threads(threads);
    thread_state = new ThreadState * [threads];
    thread_state_outgoing = new ThreadState* [threads];
    // local_send_buffer_limit = 16;
    // local_send_buffer = new MessageBuffer * [threads];
    for (IndexType t_i=0;t_i<threads;t_i++) {
        thread_state[t_i] = new ThreadState;
        thread_state_outgoing[t_i] = new ThreadState;
    //   thread_state[t_i] = (ThreadState*)numa_alloc_onnode( sizeof(ThreadState), get_socket_id(t_i));
    //   local_send_buffer[t_i] = (MessageBuffer*)numa_alloc_onnode( sizeof(MessageBuffer), get_socket_id(t_i));
    //   local_send_buffer[t_i]->init(get_socket_id(t_i));
    }
    // #pragma omp parallel for
    // for (int t_i=0;t_i<threads;t_i++) {
    //   int s_i = get_socket_id(t_i);
    //   assert(numa_run_on_node(s_i)==0);
    //   #ifdef PRINT_DEBUG_MESSAGES
    //   // printf("thread-%d bound to socket-%d\n", t_i, s_i);
    //   #endif
    // }
    #ifdef PRINT_DEBUG_MESSAGES
    // printf("threads=%d*%d\n", sockets, threads_per_socket);
    // printf("interleave on %s\n", nodestring);
    #endif

    // MPI_Comm_rank(MPI_COMM_WORLD, &partition_id);
    // MPI_Comm_size(MPI_COMM_WORLD, &partitions);
    // send_buffer = new MessageBuffer ** [partitions];
    // recv_buffer = new MessageBuffer ** [partitions];
    // for (int i=0;i<partitions;i++) {
    //   send_buffer[i] = new MessageBuffer * [sockets];
    //   recv_buffer[i] = new MessageBuffer * [sockets];
    //   for (int s_i=0;s_i<sockets;s_i++) {
    //     send_buffer[i][s_i] = (MessageBuffer*)numa_alloc_onnode( sizeof(MessageBuffer), s_i);
    //     send_buffer[i][s_i]->init(s_i);
    //     recv_buffer[i][s_i] = (MessageBuffer*)numa_alloc_onnode( sizeof(MessageBuffer), s_i);
    //     recv_buffer[i][s_i]->init(s_i);
    //   }
    // }

    // alpha = 8 * (partitions - 1);

    // MPI_Barrier(MPI_COMM_WORLD);
  }

    void load_directed(std::string datapath, std::string infopath, int input_threads, int input_coros = 2)
    {
        auto [V, E] = LoadEVFromInfoFile(infopath);
        // weight 1 mode
        // cblptr = new CoroGraph::CBList(V, E, datapath, 1);
        // cblptroutgoing = new CoroGraph::CBList(V, E, datapath, 2);

        cblptr = new CoroGraph::CBList(V, E, datapath);
        cblptroutgoing = new CoroGraph::CBList(V, E, datapath, true);


        if(cblptr == nullptr)
        {
            printf("no graph data\n");
            exit(-1);
        }
        this->threads = input_threads;
        this->coroutines = input_coros;
        SFEresult = SliceForCoroDynamic(*cblptr, threads);
        SFEresultoutgoing = SliceForCoroDynamic(*cblptroutgoing, threads);
        size_t basic_chunk = 64; 
        SFVresult = SliceForVertices(cblptr->NodeNum, threads, basic_chunk);
    }

    template<typename T>
    void fill_vertex_array(T * array, T value) {
        #pragma omp parallel for
        for (VertexID v_i=0;v_i<cblptr->NodeNum;v_i++) {
        array[v_i] = value;
        }
    }

    void init_thread_state_E()
    {
        #pragma omp parallel for
        for (IndexType t_i=0;t_i<threads;t_i++) {
        thread_state[t_i]->curr = SFEresult[t_i];
        thread_state[t_i]->end = SFEresult[t_i+1];
        thread_state[t_i]->status = WORKING;
        }
    }

    void init_thread_state_E_outgoing()
    {
        #pragma omp parallel for
        for (IndexType t_i=0;t_i<threads;t_i++) {
        thread_state[t_i]->curr = SFEresultoutgoing[t_i];
        thread_state[t_i]->end = SFEresultoutgoing[t_i+1];
        thread_state[t_i]->status = WORKING;
        }
    }

    void init_thread_state_V()
    {
        #pragma omp parallel for
        for (IndexType t_i=0;t_i<threads;t_i++) {
        thread_state[t_i]->curr = SFVresult[t_i];
        thread_state[t_i]->end = SFVresult[t_i+1];
        thread_state[t_i]->status = WORKING;
        }
    }


  template<typename R>
  R process_vertices(std::function<R(VertexID)> process, Bitmap * active) {

    R reducer = 0;
    
    size_t basic_chunk = 64; 

    init_thread_state_V();

    #pragma omp parallel reduction(+:reducer)
    {
      R local_reducer = 0;
      IndexType thread_id = omp_get_thread_num();
      while (true) {
        VertexID v_i = __sync_fetch_and_add(&thread_state[thread_id]->curr, basic_chunk);
        if (v_i >= thread_state[thread_id]->end) break;
        unsigned long word = active->data[WORD_OFFSET(v_i)];
        while (word != 0) {
          if (word & 1) {
            local_reducer += process(v_i);
          }
          v_i++;
          word = word >> 1;
        }
      }
      thread_state[thread_id]->status = STEALING;
      for (int t_offset=1;t_offset<threads;t_offset++) {
        IndexType t_i = (thread_id + t_offset) % threads;
        while (thread_state[t_i]->status!=STEALING) {
          VertexID v_i = __sync_fetch_and_add(&thread_state[t_i]->curr, basic_chunk);
          if (v_i >= thread_state[t_i]->end) continue;
          unsigned long word = active->data[WORD_OFFSET(v_i)];
          while (word != 0) {
            if (word & 1) {
              local_reducer += process(v_i);
            }
            v_i++;
            word = word >> 1;
          }
        }
      }
      reducer += local_reducer;
    }

    return reducer;
  }


  template<typename R, typename M>
  R process_edges(std::function<R(VertexID)> sparse_signal, std::function<R(VertexID)> dense_signal, Bitmap * active) {

    R reducer = 0;
    // TODO:active_edges 换个方式 或者 看一下这个开销？
    EdgeID active_edges = process_vertices<EdgeID>(
      [&](VertexID vtx){
        return (EdgeID)cblptr->NodeList[vtx].NeighboorCnt;
      },
      active
    );
    bool sparse = (active_edges < cblptr->EdgeNum / 20);
    size_t basic_chunk = 64;
    if (sparse) {
      init_thread_state_E_outgoing(); 
      #ifdef PRINT_DEBUG_MESSAGES 
        printf("sparse mode\n");
      #endif
      #pragma omp parallel reduction(+:reducer)
        {
          R local_reducer = 0;
          int thread_id = omp_get_thread_num();
          VertexID final_p_v_i = thread_state[thread_id]->end;
          while (true) {
            VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[thread_id]->curr, basic_chunk);
            if (begin_p_v_i >= final_p_v_i) break;
            VertexID end_p_v_i = begin_p_v_i + basic_chunk;
            if (end_p_v_i > final_p_v_i) {
              end_p_v_i = final_p_v_i;
            }
            for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
              if(active->get_bit(p_v_i))
                local_reducer += sparse_signal(p_v_i);
            }
          }
          thread_state[thread_id]->status = STEALING;
          for (int t_offset=1;t_offset<threads;t_offset++) {
            int t_i = (thread_id + t_offset) % threads;
            while (thread_state[t_i]->status!=STEALING) {
              VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[t_i]->curr, basic_chunk);
              if (begin_p_v_i >= thread_state[t_i]->end) break;
              VertexID end_p_v_i = begin_p_v_i + basic_chunk;
              if (end_p_v_i > thread_state[t_i]->end) {
                end_p_v_i = thread_state[t_i]->end;
              }
              for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
                if(active->get_bit(p_v_i))
                  local_reducer += sparse_signal(p_v_i);
              }
            }
          }
          reducer += local_reducer;
        }

    } else {
      init_thread_state_E();
      #ifdef PRINT_DEBUG_MESSAGES
        printf("dense mode\n");
      #endif
        #pragma omp parallel reduction(+:reducer)
        {
          R local_reducer = 0;
          int thread_id = omp_get_thread_num();
          VertexID final_p_v_i = thread_state[thread_id]->end;
          while (true) {
            VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[thread_id]->curr, basic_chunk);
            if (begin_p_v_i >= final_p_v_i) break;
            VertexID end_p_v_i = begin_p_v_i + basic_chunk;
            if (end_p_v_i > final_p_v_i) {
              end_p_v_i = final_p_v_i;
            }
            for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
              local_reducer += dense_signal(p_v_i);
            }
          }
          thread_state[thread_id]->status = STEALING;
          for (int t_offset=1;t_offset<threads;t_offset++) {
            int t_i = (thread_id + t_offset) % threads;
            while (thread_state[t_i]->status!=STEALING) {
              VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[t_i]->curr, basic_chunk);
              if (begin_p_v_i >= thread_state[t_i]->end) break;
              VertexID end_p_v_i = begin_p_v_i + basic_chunk;
              if (end_p_v_i > thread_state[t_i]->end) {
                end_p_v_i = thread_state[t_i]->end;
              }
              for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
                local_reducer += dense_signal(p_v_i);
              }
            }
          }
          reducer += local_reducer;
        }

    }

    return reducer;
  }

  template<typename R>
  R process_vertices_all_graph(std::function<R(VertexID)> process) {

    R reducer = 0;
    
    size_t basic_chunk = 64;

    init_thread_state_V();

    #pragma omp parallel reduction(+:reducer)
    {
      R local_reducer = 0;
      IndexType thread_id = omp_get_thread_num();
      while (true) {
        VertexID v_i = __sync_fetch_and_add(&thread_state[thread_id]->curr, basic_chunk);
        if (v_i >= thread_state[thread_id]->end) break;
        for(VertexID i = 0; i < basic_chunk && v_i + i < cblptr->NodeNum; ++i)
          local_reducer += process(v_i+i);
      }
      thread_state[thread_id]->status = STEALING;
      for (int t_offset=1;t_offset<threads;t_offset++) {
        IndexType t_i = (thread_id + t_offset) % threads;
        while (thread_state[t_i]->status!=STEALING) {
          VertexID v_i = __sync_fetch_and_add(&thread_state[t_i]->curr, basic_chunk);
          if (v_i >= thread_state[t_i]->end) continue;
          for(VertexID i = 0; i < basic_chunk && v_i + i < cblptr->NodeNum; ++i)
            local_reducer += process(v_i+i);
        }
      }
      reducer += local_reducer;
    }

    return reducer;
  } 

  template<typename R, typename M>
  R process_edges_all_graph(std::function<R(VertexID)> dense_signal) {

    R reducer = 0;
    size_t basic_chunk = 64;
    init_thread_state_E();
    #pragma omp parallel reduction(+:reducer)
    {
      R local_reducer = 0;
      int thread_id = omp_get_thread_num();
      VertexID final_p_v_i = thread_state[thread_id]->end;
      while (true) {
        VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[thread_id]->curr, basic_chunk);
        if (begin_p_v_i >= final_p_v_i) break;
        VertexID end_p_v_i = begin_p_v_i + basic_chunk;
        if (end_p_v_i > final_p_v_i) {
          end_p_v_i = final_p_v_i;
        }
        for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
          local_reducer += dense_signal(p_v_i);
        }
      }
      thread_state[thread_id]->status = STEALING;
      for (int t_offset=1;t_offset<threads;t_offset++) {
        int t_i = (thread_id + t_offset) % threads;
        while (thread_state[t_i]->status!=STEALING) {
          VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[t_i]->curr, basic_chunk);
          if (begin_p_v_i >= thread_state[t_i]->end) break;
          VertexID end_p_v_i = begin_p_v_i + basic_chunk;
          if (end_p_v_i > thread_state[t_i]->end) {
            end_p_v_i = thread_state[t_i]->end;
          }
          for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
            local_reducer += dense_signal(p_v_i);
          }
        }
      }
      reducer += local_reducer;
    }
    return reducer;
  }

  template<typename R, typename M>
  R coro_process_edges_all_graph(std::function<generator<void>(const CoroGraph::CBList*, VertexID, VertexID)> dense_signal) {

    R reducer = 0;
    size_t basic_chunk = 64;
    init_thread_state_E();
    #pragma omp parallel reduction(+:reducer)
    {
      R local_reducer = 0;
      int thread_id = omp_get_thread_num();
      VertexID final_p_v_i = thread_state[thread_id]->end;
      while (true) {
        VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[thread_id]->curr, basic_chunk);
        if (begin_p_v_i >= final_p_v_i) break;
        VertexID end_p_v_i = begin_p_v_i + basic_chunk;
        if (end_p_v_i > final_p_v_i) {
          end_p_v_i = final_p_v_i;
        }
        // -----coro-----
        std::vector<std::coroutine_handle<>> dense_signal_task(coroutines);
        if(end_p_v_i - begin_p_v_i > 32)
        {
          dense_signal_task[0] = dense_signal(this->cblptr, begin_p_v_i, begin_p_v_i+32).get_handle();
          dense_signal_task[1] = dense_signal(this->cblptr, begin_p_v_i+32, end_p_v_i).get_handle();
        }
        else
        {
          int half_v = (end_p_v_i - begin_p_v_i)/2;
          dense_signal_task[0] = dense_signal(this->cblptr, begin_p_v_i, begin_p_v_i+half_v).get_handle();
          dense_signal_task[1] = dense_signal(this->cblptr, begin_p_v_i+half_v, end_p_v_i).get_handle();
        }
        // for(IndexType ds = 0; ds < coroutines; ++ds)
        //   dense_signal_task[ds] = dense_signal();
        SimpleScheduler(dense_signal_task);
        // -----coro-----
      }
      thread_state[thread_id]->status = STEALING;
      for (int t_offset=1;t_offset<threads;t_offset++) {
        int t_i = (thread_id + t_offset) % threads;
        while (thread_state[t_i]->status!=STEALING) {
          VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[t_i]->curr, basic_chunk);
          if (begin_p_v_i >= thread_state[t_i]->end) break;
          VertexID end_p_v_i = begin_p_v_i + basic_chunk;
          if (end_p_v_i > thread_state[t_i]->end) {
            end_p_v_i = thread_state[t_i]->end;
          }
      //     for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
      //       local_reducer += dense_signal(p_v_i);
      //     }
          // -----coro-----
          std::vector<std::coroutine_handle<>> dense_signal_task(coroutines);
          if(end_p_v_i - begin_p_v_i > 32)
          {
            dense_signal_task[0] = dense_signal(this->cblptr, begin_p_v_i, begin_p_v_i+32).get_handle();
            dense_signal_task[1] = dense_signal(this->cblptr, begin_p_v_i+32, end_p_v_i).get_handle();
          }
          else
          {
            int half_v = (end_p_v_i - begin_p_v_i)/2;
            dense_signal_task[0] = dense_signal(this->cblptr, begin_p_v_i, begin_p_v_i+half_v).get_handle();
            dense_signal_task[1] = dense_signal(this->cblptr, begin_p_v_i+half_v, end_p_v_i).get_handle();
          }
          SimpleScheduler(dense_signal_task);
        }
      }
      // reducer += local_reducer;
    }
    return reducer;
  }

  template<typename R, typename M>
  R coro_process_edges(
    std::function<generator<void>(const CoroGraph::CBList*, VertexID, VertexID, int&)> sparse_signal, 
    std::function<generator<void>(const CoroGraph::CBList*, VertexID, VertexID, int&)> dense_signal, 
    Bitmap * active) {

    R reducer = 0;
    // TODO:active_edges 换个方式 或者 看一下这个开销？
    EdgeID active_edges = process_vertices<EdgeID>(
      [&](VertexID vtx){
        return (EdgeID)cblptr->NodeList[vtx].NeighboorCnt;
      },
      active
    );
    bool sparse = (active_edges < cblptr->EdgeNum / 20);
    size_t basic_chunk = 64;
    if (sparse) {
      init_thread_state_E_outgoing(); 
      #ifdef PRINT_DEBUG_MESSAGES 
        printf("sparse mode\n");
      #endif
      #pragma omp parallel reduction(+:reducer)
        {
          R local_reducer = 0;
          int thread_id = omp_get_thread_num();
          VertexID final_p_v_i = thread_state[thread_id]->end;
          while (true) {
            VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[thread_id]->curr, basic_chunk);
            if (begin_p_v_i >= final_p_v_i) break;
            VertexID end_p_v_i = begin_p_v_i + basic_chunk;
            if (end_p_v_i > final_p_v_i) {
              end_p_v_i = final_p_v_i;
            }
            // for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
            //   if(active->get_bit(p_v_i))
            //     local_reducer += sparse_signal(p_v_i);
            // }
            // -----coro-----
            // int local_reducer = 0;
            std::vector<std::coroutine_handle<>> sparse_signal_task(coroutines);
            if(end_p_v_i - begin_p_v_i > 32)
            {
              sparse_signal_task[0] = sparse_signal(this->cblptroutgoing, begin_p_v_i, begin_p_v_i+32, local_reducer).get_handle();
              sparse_signal_task[1] = sparse_signal(this->cblptroutgoing, begin_p_v_i+32, end_p_v_i, local_reducer).get_handle();
            }
            else
            {
              int half_v = (end_p_v_i - begin_p_v_i)/2;
              sparse_signal_task[0] = sparse_signal(this->cblptroutgoing, begin_p_v_i, begin_p_v_i+half_v, local_reducer).get_handle();
              sparse_signal_task[1] = sparse_signal(this->cblptroutgoing, begin_p_v_i+half_v, end_p_v_i, local_reducer).get_handle();
            }
            // for(IndexType ss = 0; ss < coroutines; ++ss)
            //   sparse_signal_task[ss] = sparse_signal();
            SimpleScheduler(sparse_signal_task);
            // -----coro-----
          }
          // thread_state[thread_id]->status = STEALING;
          // for (int t_offset=1;t_offset<threads;t_offset++) {
          //   int t_i = (thread_id + t_offset) % threads;
          //   while (thread_state[t_i]->status!=STEALING) {
          //     VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[t_i]->curr, basic_chunk);
          //     if (begin_p_v_i >= thread_state[t_i]->end) break;
          //     VertexID end_p_v_i = begin_p_v_i + basic_chunk;
          //     if (end_p_v_i > thread_state[t_i]->end) {
          //       end_p_v_i = thread_state[t_i]->end;
          //     }
          //     for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
          //       if(active->get_bit(p_v_i))
          //         local_reducer += sparse_signal(p_v_i);
          //     }
          //   }
          // }
          reducer += local_reducer;
        }

    } else {
      init_thread_state_E();
      #ifdef PRINT_DEBUG_MESSAGES
        printf("dense mode\n");
      #endif
        #pragma omp parallel reduction(+:reducer)
        {
          R local_reducer = 0;
          int thread_id = omp_get_thread_num();
          VertexID final_p_v_i = thread_state[thread_id]->end;
          while (true) {
            VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[thread_id]->curr, basic_chunk);
            if (begin_p_v_i >= final_p_v_i) break;
            VertexID end_p_v_i = begin_p_v_i + basic_chunk;
            if (end_p_v_i > final_p_v_i) {
              end_p_v_i = final_p_v_i;
            }
            // for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
            //   local_reducer += dense_signal(p_v_i);
            // }
            // -----coro-----
            // int local_reducer = 0;
            std::vector<std::coroutine_handle<>> dense_signal_task(coroutines);
            if(end_p_v_i - begin_p_v_i > 32)
            {
              dense_signal_task[0] = dense_signal(this->cblptr, begin_p_v_i, begin_p_v_i+32, local_reducer).get_handle();
              dense_signal_task[1] = dense_signal(this->cblptr, begin_p_v_i+32, end_p_v_i, local_reducer).get_handle();
            }
            else
            {
              int half_v = (end_p_v_i - begin_p_v_i)/2;
              dense_signal_task[0] = dense_signal(this->cblptr, begin_p_v_i, begin_p_v_i+half_v, local_reducer).get_handle();
              dense_signal_task[1] = dense_signal(this->cblptr, begin_p_v_i+half_v, end_p_v_i, local_reducer).get_handle();
            }
            // for(IndexType ds = 0; ds < coroutines; ++ds)
            //   dense_signal_task[ds] = dense_signal();
            SimpleScheduler(dense_signal_task);
            // -----coro-----
          }
          // thread_state[thread_id]->status = STEALING;
          // for (int t_offset=1;t_offset<threads;t_offset++) {
          //   int t_i = (thread_id + t_offset) % threads;
          //   while (thread_state[t_i]->status!=STEALING) {
          //     VertexID begin_p_v_i = __sync_fetch_and_add(&thread_state[t_i]->curr, basic_chunk);
          //     if (begin_p_v_i >= thread_state[t_i]->end) break;
          //     VertexID end_p_v_i = begin_p_v_i + basic_chunk;
          //     if (end_p_v_i > thread_state[t_i]->end) {
          //       end_p_v_i = thread_state[t_i]->end;
          //     }
          //     for (VertexID p_v_i = begin_p_v_i; p_v_i < end_p_v_i; p_v_i ++) {
          //       local_reducer += dense_signal(p_v_i);
          //     }
          //   }
          // }
          reducer += local_reducer;
        }

    }

    return reducer;
  }

};