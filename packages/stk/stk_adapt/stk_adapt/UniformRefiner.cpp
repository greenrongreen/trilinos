#include <exception>
#include <fstream>
#include <set>
#include <typeinfo>

#if defined( STK_HAS_MPI )
#include <mpi.h>
#endif

#include <stk_adapt/UniformRefiner.hpp>

// FIXME
// #include <stk_mesh/baseImpl/EntityImpl.hpp>
// #include <stk_mesh/base/Entity.hpp>
// FIXME


namespace stk {
  namespace adapt {

    using namespace std;
    using namespace mesh;
    using namespace percept;

    UniformRefiner::UniformRefiner(percept::PerceptMesh& eMesh, UniformRefinerPatternBase &  bp, FieldBase *proc_rank_field) : 
      m_eMesh(eMesh), m_breakPattern(), 
      m_nodeRegistry(0), 
      m_proc_rank_field(proc_rank_field), m_doRemove(true), m_ranks(), m_ignoreSideSets(false)
    {
      bp.setSubPatterns(m_breakPattern, eMesh);
    }

    BlockNamesType UniformRefiner::getBlockNames(std::string& block_name)
    {
      BlockNamesType blocks(mesh::EntityRankEnd+1u);
      if (block_name.length() == 0)
        return blocks;
      else
        {
          if (block_name.substr(0, 5) == "file:")
            {
              std::string fileName = block_name.substr(5, block_name.length()-5);
              std::ifstream file(fileName.c_str());
              while(!file.eof())
                {
                  std::string block;
                  file >> block;
                  if (block[0] != '#')
                    {
                      if (block.substr(0,6) == "block_")
                        blocks[mesh::Element].push_back(block);
                      else if (block.substr(0,8) == "surface_")
                        blocks[mesh::Face].push_back(block);
                    }
                  
                }
            }
          else if (block_name.substr(0, 5) == "list:")
            {
              std::string names = block_name.substr(5, block_name.length()-5);
              while(1)
                {
                  if (!names.length())
                    break;
                  size_t ipos = names.find(',');
                  if (ipos == std::string::npos)
                    {
                      if (names.substr(0,6) == "block_")
                        blocks[mesh::Element].push_back(names);
                      else if (names.substr(0,8) == "surface_")
                        blocks[mesh::Face].push_back(names);
                      //blocks.push_back(names);
                      break;
                    }
                  else
                    {
                      std::string n1 = names.substr(0, ipos);
                      names = names.substr(ipos+1,names.length()-(ipos+1));
                      if (n1.substr(0,6) == "block_")
                        blocks[mesh::Element].push_back(n1);
                      else if (n1.substr(0,8) == "surface_")
                        blocks[mesh::Face].push_back(n1);
                      //blocks.push_back(n1);
                    }
                }
              std::cout << "blocks = " << blocks << std::endl;
            }
          else
            {
              //blocks.push_back(block_name);

              if (block_name.substr(0,6) == "block_")
                blocks[mesh::Element].push_back(block_name);
              else if (block_name.substr(0,8) == "surface_")
                blocks[mesh::Face].push_back(block_name);
            }
        }
      return blocks;
    }

    void UniformRefiner::
    addOldElementsToPart(EntityRank rank, UniformRefinerPatternBase* breakPattern, unsigned *elementType)
    {
      EXCEPTWATCH;
      m_eMesh.getBulkData()->modification_begin();
      std::string oldPartName = breakPattern->getOldElementsPartName()+toString(rank);
      mesh::Part *oldPart = m_eMesh.getMetaData()->get_part(oldPartName);
      if (!oldPart)
        {
          std::cout << "oldPartName= " << oldPartName << std::endl;
          throw std::runtime_error("oldpart is null");
        }

      mesh::PartVector add_parts(1, oldPart);
      mesh::PartVector remove_parts;
      mesh::Selector on_locally_owned_part =  ( m_eMesh.getMetaData()->locally_owned_part() );

      std::vector<Entity*> elems;
      const vector<Bucket*> & buckets = m_eMesh.getBulkData()->buckets( rank );

      for ( vector<Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k ) 
        {
          if (on_locally_owned_part(**k)) 
            {
              Bucket & bucket = **k ;
              const CellTopologyData * const bucket_cell_topo_data = stk::mesh::get_cell_topology(bucket);
              shards::CellTopology topo(bucket_cell_topo_data);

              const unsigned num_elements_in_bucket = bucket.size();

              for (unsigned i_element = 0; i_element < num_elements_in_bucket; i_element++)
                {
                  EXCEPTWATCH;
                  Entity& element = bucket[i_element];
                  if (&element == 0)
                    {
                      std::cout << "element = 0" << std::endl;
                      throw std::runtime_error("element = 0");
                      //exit(1);
                    }

                  
                  if (elementType && (topo.getKey() != *elementType))
                    {
                    }
                  else
                    {
                      elems.push_back(&element);
                      //std::cout << "tmp adding to oldParts = " << element << std::endl;
                    }
                }
            }
        }

      for (unsigned ielem=0; ielem < elems.size(); ielem++)
        {
          //std::cout << "tmp element = " << *elems[ielem] << std::endl;
          m_eMesh.getBulkData()->change_entity_parts( *elems[ielem], add_parts, remove_parts );
        }

      m_eMesh.getBulkData()->modification_end();
    }

    void UniformRefiner::
    trace_print(std::string msg)
    {
      if (TRACE_STAGE_PRINT) {
        size_t heap_in_Mb = 0;
        size_t memory_in_Mb = Util::memory(heap_in_Mb);
        memory_in_Mb = memory_in_Mb / (1024*1024);
        heap_in_Mb = heap_in_Mb / (1024*1024);

        double cpu = Util::cpu_time();
        std::cout
          << msg
          << " mem= " << memory_in_Mb << " [Mb] "
          //<< " heap= " << heap_in_Mb << " [Mb] "
          << " cpu_time= " << cpu/(60.) << " [min] "
          <<std::endl;
      }

    }


    struct myVec
    {
      double *data;
      int len;
      int res;
    };

    void UniformRefiner::
    doBreak()
    {
      EXCEPTWATCH;

      /**/                                                TRACE_PRINT( "UniformRefiner:doBreak start...");

      if (0)
        {
          std::cout 
            << "sizeof(myVec) = " << sizeof(myVec) << " "
            << "sizeof(Relation) = " << sizeof(stk::mesh::Relation) << " "
            << "sizeof(Entity) = " << sizeof(stk::mesh::Entity) << " "
            << "sizeof(EntityImpl) = " << sizeof(stk::mesh::impl::EntityImpl) << " "
            << "\nsizeof(EntityKey) = " << sizeof(stk::mesh::EntityKey) << " "
            << "\nsizeof(RelationVector) = " << sizeof(stk::mesh::RelationVector) << " "
            << "\nsizeof(EntityCommInfoVector) = " << sizeof(stk::mesh::EntityCommInfoVector) << " "
            << "\nsizeof(Bucket *) = " << sizeof(stk::mesh::Bucket *) << " "
            << "\nsizeof(unsigned) = " << sizeof(unsigned) << " "
            << "\nsizeof(size_t) = " << sizeof(size_t) << " "
            << "\nsizeof(EntityModificationLog) = " << sizeof(EntityModificationLog) << std::endl;

        }

      //m_nodeRegistry = new NodeRegistry(m_eMesh);
      NodeRegistry nr (m_eMesh);
      m_nodeRegistry = &nr;

      //stk::ParallelMachine pm = m_eMesh.getBulkData()->parallel();
      //unsigned proc_size = parallel_machine_size(pm);
      //unsigned proc_rank = parallel_machine_rank(pm);

      CommDataType buffer_entry;

      BulkData& bulkData = *m_eMesh.getBulkData();
      static SubDimCellData empty_SubDimCellData;

      // color elements
#if 0
      struct EntityExcluder
      {
        virtual bool exclude(Entity& element) = 0;
      };
#endif

      std::vector<EntityRank> ranks;

      // do elements first, then any faces or edge elements
      for (unsigned ibp = 0; ibp < m_breakPattern.size(); ibp++)
        {
          if (m_breakPattern[ibp])
            {
              EntityRank irank = m_breakPattern[ibp]->getPrimaryEntityRank();
              EntityRank irank_prev = EntityRankEnd;
              if (ibp > 0) irank_prev = m_breakPattern[ibp-1]->getPrimaryEntityRank();
              if (irank > irank_prev)
                {
                  throw std::logic_error("m_breakPattern: must be in decreasing order of rank");
                }
              ranks.push_back(irank);
              if (m_doRemove)
                {
                  unsigned elementType = m_breakPattern[ibp]->getFromTypeKey();
                  addOldElementsToPart(irank, m_breakPattern[ibp], &elementType);
                }
            }
          else
            {
              std::cout << "ibp = " << ibp << std::endl;
              throw std::logic_error("m_breakPattern is null");
            }
        }
      
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // do the top level, all elements of this rank operation
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      unsigned num_elem_not_ghost_0 = 0; 

      ///////////////////////////////////////////////////////////
      ///// Do the mesh coloring step for each type of element
      ///////////////////////////////////////////////////////////

      vector< vector< ColorerSetType > > elementColorsByType = vector < vector< ColorerSetType > > (ranks.size());

      for (unsigned irank = 0; irank < ranks.size(); irank++)
        {
          {
            EXCEPTWATCH;
            unsigned elementType = m_breakPattern[irank]->getFromTypeKey();
            shards::CellTopology cell_topo(m_breakPattern[irank]->getFromTopology());

            if ( TRACE_STAGE_PRINT) std::cout << "tmp UniformRefiner:: irank = " << irank << " ranks[irank] = " << ranks[irank] 
                                             << " elementType= " << elementType << std::endl;

            std::vector<EntityRank> ranks_one(1, ranks[irank]);

            // this gives a list of colored elements for this element type only
            Colorer meshColorerThisTypeOnly(elementColorsByType[irank], ranks_one);   TRACE_PRINT("UniformRefiner: Color mesh (all top level rank elements)... ");
            meshColorerThisTypeOnly.color(m_eMesh, &elementType);                     TRACE_PRINT("UniformRefiner: Color mesh (all top level rank elements)...done ");

            if (0 && elementColorsByType[irank].size() == 0)
              {
                std::cout << "WARNING: no elements found of this type: " << cell_topo.getName() << " key= " << elementType << std::endl;
              }

          }

        }

      // FIXME warn if a topology shows up without a break pattern

      ///////////////////////////////////////////////////////////
      /////  // start top-level ranks
      ///////////////////////////////////////////////////////////

      {   // start top-level ranks


        ///////////////////////////////////////////////////////////
        /////  // node registration step
        ///////////////////////////////////////////////////////////

        {  // node registration step
          EXCEPTWATCH;
          m_nodeRegistry->initialize();                           /**/  TRACE_PRINT("UniformRefiner: beginRegistration (top-level rank)... ");

#if NEW_FIX_ELEMENT_SIDES
          m_eMesh.adapt_parent_to_child_relations().clear();
#endif

          // register non-ghosted elements needs for new nodes, parallel create new nodes
          m_nodeRegistry->beginRegistration();
      
          for (unsigned irank = 0; irank < ranks.size(); irank++)
            {
              if (ranks[irank] == ranks[0])
                {
                  EXCEPTWATCH;

                  vector< ColorerSetType >& elementColors = elementColorsByType[irank];

                  vector<NeededEntityType> needed_entity_ranks;
                  m_breakPattern[irank]->fillNeededEntities(needed_entity_ranks);
                  unsigned num_elem_not_ghost_0_incr = doForAllElements(ranks[irank], &NodeRegistry::registerNeedNewNode, elementColors, needed_entity_ranks);
                  num_elem_not_ghost_0 += num_elem_not_ghost_0_incr;
                }
            }

          m_nodeRegistry->endRegistration();                    /**/  TRACE_PRINT("UniformRefiner: endRegistration (top-level rank)... ");
        }

        ///////////////////////////////////////////////////////////
        /////  Check for remote
        ///////////////////////////////////////////////////////////

        {   // beginCheckForRemote()
          EXCEPTWATCH;

          /**/                                                        TRACE_PRINT("UniformRefiner: beginCheckForRemote (top-level rank)... ");

          // now register ghosted elements needs for new nodes (this does a pack operation)
          m_nodeRegistry->beginCheckForRemote();
          unsigned num_elem = 0;
          for (unsigned irank = 0; irank < ranks.size(); irank++)
            {
              if (ranks[irank] == ranks[0])
                {
                  EXCEPTWATCH;

                  vector< ColorerSetType >& elementColors = elementColorsByType[irank];

                  vector<NeededEntityType> needed_entity_ranks;
                  m_breakPattern[irank]->fillNeededEntities(needed_entity_ranks);

                  num_elem = doForAllElements(ranks[irank], &NodeRegistry::checkForRemote, elementColors, needed_entity_ranks, false, false);
                }
            }
          m_nodeRegistry->endCheckForRemote();                /**/   TRACE_PRINT("UniformRefiner: endCheckForRemote (top-level rank)... ");

          if (0)
            {
              std::cout << "num_elem= " << num_elem << std::endl;
              MPI_Barrier( MPI_COMM_WORLD );
              std::cout << "P["<< m_eMesh.getRank() 
                        <<"] ========================================================================================================================" << std::endl;
              m_nodeRegistry->checkDB();
              MPI_Barrier( MPI_COMM_WORLD );
              std::cout << "P["<< m_eMesh.getRank() 
                        <<"] ========================================================================================================================" << std::endl;
            }

        }

        ///////////////////////////////////////////////////////////
        /////  Get from remote
        ///////////////////////////////////////////////////////////
        /// communicate all-to-all the new node creation information which also updates the node registry so it can
        /// be queried locally now for any ghost or non-ghost element

        { // get from remote

          EXCEPTWATCH;

          /**/                                                        TRACE_PRINT("UniformRefiner: beginGetFromRemote (top-level rank)... ");
          m_nodeRegistry->beginGetFromRemote();
          unsigned num_elem = 0;
          for (unsigned irank = 0; irank < ranks.size(); irank++)
            {
              if (ranks[irank] == ranks[0])
                {
                  EXCEPTWATCH;

                  vector< ColorerSetType >& elementColors = elementColorsByType[irank];

                  vector<NeededEntityType> needed_entity_ranks;
                  m_breakPattern[irank]->fillNeededEntities(needed_entity_ranks);

                  num_elem = doForAllElements(ranks[irank], &NodeRegistry::getFromRemote, elementColors, needed_entity_ranks, false, false);
                }
            }

          m_nodeRegistry->endGetFromRemote();                    /**/  TRACE_PRINT("UniformRefiner: endGetFromRemote (top-level rank)... ");

          //stk::diag::printTimersTable(std::cout, perceptTimer(), stk::diag::METRICS_ALL, false);

          if (0)
            {
              std::cout << "num_elem= " << num_elem << std::endl;
              MPI_Barrier( MPI_COMM_WORLD );
              std::cout << "P["<< m_eMesh.getRank() 
                        <<"] ========================================================================================================================" << std::endl;
              m_nodeRegistry->checkDB();
              MPI_Barrier( MPI_COMM_WORLD );
              std::cout << "P["<< m_eMesh.getRank() 
                        <<"] ========================================================================================================================" << std::endl;
            }
        }  // get from remote
      } // start top-level ranks

      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // for each element type, in top-down rank order, do the rest of the refinement operations
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      for (unsigned irank = 0; irank < ranks.size(); irank++)
        {
          EXCEPTWATCH;

          unsigned elementType = m_breakPattern[irank]->getFromTypeKey();
          if (TRACE_STAGE_PRINT) 
            std::cout << "tmp UniformRefiner:: irank = " << irank 
                      << " ranks[irank] = " << ranks[irank] << " elementType= " << elementType << std::endl;

          std::vector<EntityRank> ranks_one(1, ranks[irank]);

          vector< ColorerSetType >& elementColors = elementColorsByType[irank];

          // loop over elements, build faces, edges in threaded mode (guaranteed no mem conflicts)
          // (note: invoke UniformRefinerPattern: what entities are needed)
          vector<NeededEntityType> needed_entity_ranks;
          m_breakPattern[irank]->fillNeededEntities(needed_entity_ranks);

          vector<Entity *> new_elements;

          bulkData.modification_begin(); 

          {
            EXCEPTWATCH;

            // count num new elements needed on this proc (served by UniformRefinerPattern)
            bool count_only = true;  
            /**/                                                TRACE_PRINT("UniformRefiner: registerNeedNewNode count_only(true) ranks[irank]==ranks[0]... ");
            unsigned num_elem_not_ghost = doForAllElements(ranks[irank], &NodeRegistry::registerNeedNewNode, elementColors, needed_entity_ranks, count_only);
            /**/                                                TRACE_PRINT("UniformRefiner: registerNeedNewNode count_only(true) ranks[irank]==ranks[0]... done ");

            unsigned num_elem_needed = num_elem_not_ghost * m_breakPattern[irank]->getNumNewElemPerElem();

            if (0 && num_elem_not_ghost != num_elem_not_ghost_0) 
              {
                std::cout << "num_elem_not_ghost_0 = " << num_elem_not_ghost_0 << " num_elem_not_ghost= " << num_elem_not_ghost << std::endl;
                //exit(1);
                throw std::runtime_error("num_elem_not_ghost_0 != num_elem_not_ghost");
              }

            // create new entities on this proc
            m_nodeRegistry->beginLocalMeshMods();
            new_elements.resize(0);                                                /**/ TRACE_PRINT("UniformRefiner: createEntities... ranks[irank]==ranks[0] ");
            m_eMesh.createEntities( ranks[irank], num_elem_needed, new_elements); /**/ TRACE_PRINT("UniformRefiner: createEntities... ranks[irank]==ranks[0] done ");
            m_nodeRegistry->endLocalMeshMods();

          } 


          /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          ///  Global element ops: here's where we e.g. connect the new elements by declaring new relations
          /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          /**/                                                TRACE_PRINT("UniformRefiner: createElementsAndNodesAndConnectLocal... ");
          /**/                                                TRACE_CPU_TIME_AND_MEM_0(CONNECT_LOCAL);

          createElementsAndNodesAndConnectLocal(ranks[irank], m_breakPattern[irank], elementColors, needed_entity_ranks, new_elements);

          /**/                                                TRACE_CPU_TIME_AND_MEM_1(CONNECT_LOCAL);
          /**/                                                TRACE_PRINT("UniformRefiner: createElementsAndNodesAndConnectLocal...done ");

          /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          ///  Global node loop operations:  this is where we perform ops like adding new nodes to the right parts, interpolating fields, etc.
          /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          /**/                                                TRACE_PRINT("UniformRefiner: addToExistingParts [etc.]... ");
#if !STK_ADAPT_URP_LOCAL_NODE_COMPS
          {
            EXCEPTWATCH;
            //if (!irank)
            if (ranks[irank] == ranks[0])
              {
                m_nodeRegistry->addToExistingParts();
                m_nodeRegistry->makeCentroid(m_eMesh.getCoordinatesField());
                m_nodeRegistry->interpolateFields();
              }
          }
#endif
          /**/                                                TRACE_PRINT("UniformRefiner: addToExistingParts [etc.] ...done ");

          // this is for testing removing old elements as early as possible for memory reasons
          // FIXME - remove old elements on the fly?
          if (0 && m_doRemove)
            {
              EXCEPTWATCH;

              /**/                                                TRACE_PRINT( "UniformRefiner: remove old elements...start " );

              removeOldElements(ranks[irank], m_breakPattern[irank]);
              renameNewParts(ranks[irank], m_breakPattern[irank]);
              fixSurfaceAndEdgeSetNames(ranks[irank], m_breakPattern[irank]);

              /**/                                                TRACE_PRINT( "UniformRefiner: remove old elements...done " );
            }

          if (TRACE_STAGE_PRINT && !m_eMesh.getRank()) {
            Util::trace_cpu_time_and_mem_print(CONNECT_LOCAL, "CONNECT_LOCAL");
            Util::trace_cpu_time_and_mem_print(CONNECT_LOCAL_createNewNeededNodes, "CONNECT_LOCAL_createNewNeededNodes");
            Util::trace_cpu_time_and_mem_print(CONNECT_LOCAL_createNewElements, "CONNECT_LOCAL_createNewElements");
            Util::trace_cpu_time_and_mem_print(CONNECT_LOCAL_URP_createOrGetNode, "CONNECT_LOCAL_URP_createOrGetNode");
            Util::trace_cpu_time_and_mem_print(CONNECT_LOCAL_URP_declare_relation, "CONNECT_LOCAL_URP_declare_relation");
          }

          /**/                                                TRACE_PRINT("UniformRefiner: modification_end...start... ");
          bulkData.modification_end();
          /**/                                                TRACE_PRINT("UniformRefiner: modification_end...done ");

        } // irank


      if (m_doRemove)
        {
          EXCEPTWATCH;

          bulkData.modification_begin();

          /***********************/                           TRACE_PRINT("UniformRefiner: fixElementSides1 ");
#if NEW_FIX_ELEMENT_SIDES
          fixElementSides1();
          m_eMesh.adapt_parent_to_child_relations().clear();
#endif
          /***********************/                           TRACE_PRINT("UniformRefiner: fixElementSides1...done ");

          for (unsigned irank = 0; irank < ranks.size(); irank++)
            {
              removeOldElements(ranks[irank], m_breakPattern[irank]);
              renameNewParts(ranks[irank], m_breakPattern[irank]);
              fixSurfaceAndEdgeSetNames(ranks[irank], m_breakPattern[irank]);
            } 
 
          /**/                                                TRACE_PRINT("UniformRefiner: fixElementSides ");
#if !NEW_FIX_ELEMENT_SIDES
          fixElementSides();
#endif
          /**/                                                TRACE_PRINT("UniformRefiner: fixElementSides...done ");

          /**/                                                TRACE_PRINT("UniformRefiner: modification_end...start ");
          bulkData.modification_end();
          /**/                                                TRACE_PRINT("UniformRefiner: modification_end...done ");
        }


      /**/                                                TRACE_PRINT( "UniformRefiner:doBreak ... done");


      //std::cout << "tmp m_nodeRegistry.m_gee_cnt= " << m_nodeRegistry->m_gee_cnt << std::endl;
      //std::cout << "tmp m_nodeRegistry.m_gen_cnt= " << m_nodeRegistry->m_gen_cnt << std::endl;

    } // doBreak

    unsigned UniformRefiner::
    doForAllElements(EntityRank rank, NodeRegistry::ElementFunctionPrototype function, 
                     vector< ColorerSetType >& elementColors, vector<NeededEntityType>& needed_entity_ranks,
                     bool only_count, bool doAllElements)
    //bool only_count=false, bool doAllElements=true);

    {
      EXCEPTWATCH;
      unsigned num_elem = 0;
      for (unsigned icolor = 0; icolor < elementColors.size(); icolor++)
        {
          if (elementColors[icolor].size() == 0)
            {
              std::cout << "tmp doForAllElements elementColors size = 0!!!" << std::endl;
              continue;
            }

          //Entity* first_element_p = *(elementColors[icolor].begin());
          //const CellTopologyData * const cell_topo_data = get_cell_topology(*first_element_p);

          // do in threaded mode FIXME
          for (ColorerSetType::iterator iele = elementColors[icolor].begin();
               iele !=  elementColors[icolor].end(); 
               iele++)
            {
              const Entity * element_p =  *iele;

              const Entity& element = * element_p;

              bool elementIsGhost = m_eMesh.isGhostElement(element);
              if (!elementIsGhost) 
                ++num_elem;

              if (!only_count && (doAllElements || elementIsGhost))
                {
                  m_nodeRegistry->doForAllSubEntities(function, element, needed_entity_ranks);
                }

            } // elements in this color
        } // icolor

      return num_elem;
    }

    void UniformRefiner::
    createElementsAndNodesAndConnectLocal(EntityRank rank, UniformRefinerPatternBase *breakPattern,
                                          vector< ColorerSetType >& elementColors,   vector<NeededEntityType>& needed_entity_ranks,  
                                          vector<Entity *>& new_elements_pool)
    {
      EXCEPTWATCH;
      static NewSubEntityNodesType s_new_sub_entity_nodes(mesh::EntityRankEnd);

      NewSubEntityNodesType& new_sub_entity_nodes = s_new_sub_entity_nodes;

      vector<Entity *>::iterator element_pool_it = new_elements_pool.begin();

      int jele = 0;
      // create new elements and connect them up

      for (unsigned icolor = 0; icolor < elementColors.size(); icolor++)
        {
          //std::string msg = 
          TRACE_PRINT(  "UniformRefiner:connectLocal color= " + percept::toString(icolor) + " [ " +
                        (percept::toString (((double)icolor)/((double)elementColors.size())*100 )).substr(0,4) + " %] ");
          
          if (elementColors[icolor].size() == 0)
            {
              std::cout << "tmp elementColors size = 0!!!" << std::endl;
              continue;
            }

          Entity* first_element_p = *(elementColors[icolor].begin());

          const CellTopologyData * const cell_topo_data = get_cell_topology(*first_element_p);
          CellTopology cell_topo(cell_topo_data);
          
          // do in threaded mode FIXME
          for (ColorerSetType::iterator iele = elementColors[icolor].begin();  iele !=  elementColors[icolor].end();  iele++)
            {
              Entity* element_p = *iele;
              if (!element_p) 
                {
                  throw std::runtime_error("UniformRefiner::connectLocal");
                }

              Entity& element = * element_p;

              if (m_proc_rank_field && rank == mesh::Element)
                {
                  //exit(1);  // FIXME FIXME FIXME
                  double *fdata = stk::mesh::field_data( *static_cast<const ScalarFieldType *>(m_proc_rank_field) , element );
                  //fdata[0] = double(m_eMesh.getRank());
                  fdata[0] = double(element.owner_rank());
                }


              if (!m_eMesh.isGhostElement(element))
                {
                  //std::cout << "P["<< m_eMesh.getRank() << "] element.owner_rank() = " << element.owner_rank() << std::endl;
                  /**/                                                TRACE_CPU_TIME_AND_MEM_0(CONNECT_LOCAL_createNewNeededNodes);
                  if (createNewNeededNodeIds(cell_topo_data, element, needed_entity_ranks, new_sub_entity_nodes))
                    {
                      std::cout << "typeid= " << typeid(*breakPattern).name() << std::endl;
                      //breakPattern;
                      throw std::logic_error("needed_entity_ranks[ineed_ent].second");
                      
                    }
                  /**/                                                TRACE_CPU_TIME_AND_MEM_1(CONNECT_LOCAL_createNewNeededNodes);

                  /**/                                                TRACE_CPU_TIME_AND_MEM_0(CONNECT_LOCAL_createNewElements);
                  breakPattern->createNewElements(m_eMesh, *m_nodeRegistry, element, new_sub_entity_nodes, element_pool_it, m_proc_rank_field);
                  /**/                                                TRACE_CPU_TIME_AND_MEM_1(CONNECT_LOCAL_createNewElements);
                }
              
              ++jele;
            }
        }
    }


    /** Creates a map of element sides to their higher-dimensional base elements
     */

#define EXTRA_PRINT_UR_BESDB 0

    void UniformRefiner::
    buildElementSideDB(SubDimCellToDataMap& cell_2_data_map)
    {

    }


    /** @deprecated */
    void UniformRefiner::
    fixElementSides()
    {
    }

    void UniformRefiner::
    fixElementSides1()
    {
      EXCEPTWATCH;
      if (getIgnoreSideSets()) return;

      if (m_eMesh.getSpatialDim() == 3)
        {
          fixElementSides1(mesh::Face);
          //checkFixElementSides(mesh::Face, mesh::Element);
        }
      // FIXME
      else if (m_eMesh.getSpatialDim() == 2)
        {
          fixElementSides1(mesh::Edge);
        }
    }


    /** Sets orientations and associativity of elements to sub-dimensional faces/edges after refinement.
     */
#define EXTRA_PRINT_UR_FES 0

    // new version 011411 srk
    /**
     *                                                                               
     *        _____                                                                       
     *       |  |  |   |                                                                
     *       |__|__|  _|_                                                               
     *       |  |  |   |                                                                    
     *       |__|__|   |                                                                    
     *                                                                               
     *                                                                               
     *                                                                               
     *                                                                               
     *                                                                               
     *                                                                               
     *                                                                               
     *                                                                               
     *                                                                               
     *                                                                               
     *                                                                               
     *
     */

    void UniformRefiner::
    fixElementSides1(EntityRank side_rank)
    {
      EXCEPTWATCH;

      bool notFound = true;
      for (unsigned ibp = 0; ibp < m_breakPattern.size(); ibp++)
        {
          // only check the side elements
          if (m_breakPattern[ibp]->getPrimaryEntityRank() == side_rank)
            {
              notFound = false;
            }
        }
      if (notFound)
        {
          std::cout << "UniformRefiner::fixElementSides: missing sub-dim break pattern - logic error\n"
            " ---- for this refinement pattern to be able to handle sidesets and edgesets you must provide the sub-dim break pattern\n"
            " ---- or you must set the setIgnoreSideSets() flag " << std::endl;
          throw std::logic_error("UniformRefiner::fixElementSides1: missing sub-dim break pattern - logic error");
          return;
        }

      SameRankRelation& parent_child = m_eMesh.adapt_parent_to_child_relations();
      SameRankRelation::iterator pc_it;
      for (pc_it = parent_child.begin(); pc_it != parent_child.end(); pc_it++)
        {
          const SameRankRelationKey& parent = pc_it->first;
          SameRankRelationValue& child_vector = pc_it->second;

          shards::CellTopology parent_topo(stk::mesh::get_cell_topology(*parent));
          //unsigned parent_nsides = (unsigned)parent_topo.getSideCount();

          for (unsigned i_child = 0; i_child < child_vector.size(); i_child++)
            {
              Entity *child = child_vector[i_child];
              //mesh::PairIterRelation child_sides = child->relations(side_rank);
              if (!child)
                {
                  std::cout << "fixElementSides1: child == null, i_child= " << i_child << " nchild= " << child_vector.size() << std::endl;
                  throw std::runtime_error("fixElementSides1: child == null");
                }

              shards::CellTopology child_topo(stk::mesh::get_cell_topology(*child));
              unsigned child_nsides = (unsigned)child_topo.getSideCount();

              // if parent has any side relations, check if any of the sides' children match the parent's children's faces
              mesh::PairIterRelation parent_sides = parent->relations(side_rank);
              mesh::PairIterRelation side_to_parent = parent->relations(mesh::Element);

              //std::cout << "tmp here 1 child_nsides= " << child_nsides 
              //          << " parent_sides.size()=" << parent_sides.size() <<  " side_to_parent.size() = " << side_to_parent.size() << std::endl;

              for (unsigned i_parent_side = 0; i_parent_side < parent_sides.size(); i_parent_side++)
                {
                  Entity *parent_side = parent_sides[i_parent_side].entity();
                  //unsigned local_parent_side_id = parent_sides[i_parent_side].identifier();

                  SameRankRelationValue& parent_side_children = m_eMesh.adapt_parent_to_child_relations()[parent_side];

                  //std::cout << "tmp here 2 parent_side_children.size() = " << parent_side_children.size()
                  //          << std::endl;

                  for (unsigned i_parent_side_child = 0; i_parent_side_child < parent_side_children.size(); i_parent_side_child++)
                    {
                      Entity *parent_side_child = parent_side_children[i_parent_side_child];

                      //std::cout << "tmp here 3 parent_side_child = " << *parent_side_child
                      //      << std::endl;

                      int permIndex = -1;
                      int permPolarity = 1;

                      // use of i_parent_side here implies that the children's sides match up with the parents, this could be untrue - 
                      //  then will require a search through all child faces 
                      // NOTE: have to search over child faces due to different topology cases - if parent & child have same topology,
                      //   we can save a few ops here TODO FIXME
                      unsigned k_child_side = 0;
#if 0
                      boolean sameTopology = false; // FIXME - get this from the break pattern
                      if (sameTopology)
                        {
                          PerceptMesh::element_side_permutation(*child, *parent_side_child, k_child_side, permIndex, permPolarity);
                        }
#endif


                      if (permIndex < 0)
                        {
                          // try search
                          for (unsigned j_child_side = 0; j_child_side < child_nsides; j_child_side++)
                            {
                              PerceptMesh::element_side_permutation(*child, *parent_side_child, j_child_side, permIndex, permPolarity);
                              if (0)
                                std::cout << "tmp j_child_side = " << j_child_side << " permIndex= " << permIndex 
                                          << " child= " << *child
                                          << " parent_side_child= " << *parent_side_child
                                          <<  std::endl;

                              if (permIndex >= 0)
                                {
                                  k_child_side = j_child_side;
                                  break;
                                }
                            }
                        }

                      if (permIndex >= 0)
                        {
                          if (0)
                            std::cout << "tmp decl rel permIndex= " << permIndex 
                                      << " child= " << *child
                                      << " parent_side_child= " << *parent_side_child
                                      <<  std::endl;
                          m_eMesh.getBulkData()->declare_relation(*child, *parent_side_child, k_child_side);
                          PerceptMesh::element_side_permutation(*child, *parent_side_child, k_child_side, permIndex, permPolarity);
                        }
                      else
                        {
                          // error condition?
                          //throw std::runtime_error("fixElementSides1: couldn't find a matching face");
                        }
                    }
                }
            }
        }
    }

    void UniformRefiner::
    fixElementSides(EntityRank side_rank)
    {

    }

#undef EXTRA_PRINT_UR_FES 

    /** Sets orientations and associativity of elements to sub-dimensional faces/edges after refinement.
     */
    void UniformRefiner::
    checkFixElementSides(EntityRank side_rank, EntityRank elem_rank)
    {
    }

    void UniformRefiner::
    removeOldElements(EntityRank rank, UniformRefinerPatternBase* breakPattern)
    {
      EXCEPTWATCH;

#if 1
      const mesh::Part *oldPart = m_eMesh.getPart(breakPattern->getOldElementsPartName()+toString(rank));

      //std::cout << "tmp removeOldElements::name= " << oldPart->name() << " for rank= " << rank << std::endl;

      if (!oldPart)
        {
          std::cout << "name= " << breakPattern->getOldElementsPartName()+toString(rank) << std::endl;
          throw std::runtime_error("oldPart is null");
        }
      mesh::Selector removePartSelector (*oldPart);
      
#else

      mesh::Selector toPartSelector = mesh::selectUnion( breakPattern->getToParts() );
#endif

      const vector<Bucket*> & buckets = m_eMesh.getBulkData()->buckets( rank );

      elements_to_be_destroyed_type elements_to_be_destroyed;

#if UNIFORM_REF_REMOVE_OLD_STD_VECTOR
      unsigned nel = 0u;
      for ( vector<Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k ) 
        {
          if (removePartSelector(**k)) 
            {
              Bucket & bucket = **k ;
              const unsigned num_elements_in_bucket = bucket.size();

              nel += num_elements_in_bucket;
            }
        }
      elements_to_be_destroyed.reserve(nel);
#endif
      
      for ( vector<Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k ) 
        {
          if (removePartSelector(**k)) 
            {
              Bucket & bucket = **k ;
              const unsigned num_elements_in_bucket = bucket.size();

              if (0)
                {
                  std::string str;
                  PartVector pv;
                  bucket.supersets(pv);
                  for (unsigned ip = 0; ip < pv.size(); ip++)
                    {
                      str += " "+pv[ip]->name();
                    }
                  std::cout << "P[" << m_eMesh.getRank() << "] removing elements in bucket of parts: " << str << std::endl;
                }
              for (unsigned iElement = 0; iElement < num_elements_in_bucket; iElement++)
                {
                  Entity& element = bucket[iElement];
                  Entity* element_p = &element;

                  if (!m_eMesh.isGhostElement(element))
                    {
#if UNIFORM_REF_REMOVE_OLD_STD_VECTOR
                      elements_to_be_destroyed.push_back(element_p);
#else
                      elements_to_be_destroyed.insert(element_p);
#endif
                      //std::cout << "tmp removing elem = " << *element_p << std::endl;
                    }
                }
            }
        }
      removeOldElements(elements_to_be_destroyed);

    }

    void UniformRefiner::removeOldElements(elements_to_be_destroyed_type& elements_to_be_destroyed)
    {
      for (elements_to_be_destroyed_type::iterator itbd = elements_to_be_destroyed.begin(); itbd != elements_to_be_destroyed.end();  ++itbd)
        {
          Entity *element_p = *itbd;
          if ( ! m_eMesh.getBulkData()->destroy_entity( element_p ) )
            {
              throw std::logic_error("UniformRefiner::removeOldElements couldn't remove element");
            }
        }
    }

    /// fix names of surfaces (changing for example surface_hex8_quad4 to surface_tet4_tri3)
    void UniformRefiner::
    fixSurfaceAndEdgeSetNames(EntityRank rank, UniformRefinerPatternBase* breakPattern)
    {
      EXCEPTWATCH;
      PartVector toParts = breakPattern->getToParts();

      //std::cout << "toParts.size()= " << toParts.size() << " typeid= " << typeid(*breakPattern).name()  << std::endl;

      for (unsigned i_part = 0; i_part < toParts.size(); i_part++)
        {
          //const std::string & partName = toParts[i_part]->name();
          std::string * toPartName_p = const_cast<std::string *> (&toParts[i_part]->name());

          std::string toPartName = toParts[i_part]->name();
          if ( toPartName.find("surface_", 0) == std::string::npos)
            {
              if (0) std::cout << "tmp fixSurfaceAndEdgeSetNames:: skipping toPartName= " << toPartName << " typeid= " << typeid(*breakPattern).name()  << std::endl;
              continue;
            }
          
          std::string newToPartName = toPartName;

          StringStringMap::iterator map_it;
          StringStringMap str_map =  breakPattern->fixSurfaceAndEdgeSetNamesMap();
          if (0) std::cout << "tmp fixSurfaceAndEdgeSetNamesMap:: str_map.size()= " << str_map.size() 
            //<< " " << breakPattern->getFromTopoPartName() << "__" << breakPattern->getToTopoPartName()
                           << " typeid= " << typeid(*breakPattern).name() 
                           << std::endl;

          for (map_it = str_map.begin(); map_it != str_map.end(); map_it++)
            {
              std::string from_str = map_it->first;
              std::string to_str = map_it->second;
              Util::replace(newToPartName, from_str, to_str);
              if (0) 
                std::cout << "tmp fixSurfaceAndEdgeSetNamesMap: old= " << toPartName << " new= " << newToPartName << std::endl;
            }

          *toPartName_p = newToPartName;

          if (0)
            std::cout << "tmp fixSurfaceAndEdgeSetNamesMap:: P[" << m_eMesh.getRank() << "] new part name= " << toParts[i_part]->name() 
                      << " old part name = " << toPartName
                      << std::endl;
        }
    }

    // FIXME this is a hack to rename parts
    void UniformRefiner::
    renameNewParts(EntityRank rank, UniformRefinerPatternBase* breakPattern)
    {
      EXCEPTWATCH;
      PartVector toParts = breakPattern->getToParts();
      PartVector fromParts = breakPattern->getFromParts();

      if (0)
        {
          for (unsigned i_part = 0; i_part < toParts.size(); i_part++)
            {
              std::cout << "tmp toParts[i_part]->name() = " << toParts[i_part]->name() 
                        << " fromParts[i_part]->name() = " << fromParts[i_part]->name()  << std::endl;
            }

        }

      // Renames as follows:
      //   originalPartName -> originalPartName_uo_1000    The original part holds the elements to be converted, and is renamed to be the "old" part
      //   originalPartName_urpconv -> originalPartName    The new part has the same name as the original part with urpconv appended, which
      //                                                      is then changed back to the original part name
      //
      // So, after the renaming, the original part name holds the new elements, and the original elements are 
      //   in the part with the original name appended with _uo_1000.  These parts are ignored on subsequent input.
      //
      for (unsigned i_part = 0; i_part < toParts.size(); i_part++)
        {
          if (0) std::cout << "tmp before: fromPartName= " << fromParts[i_part]->name() << " toPartName= " << toParts[i_part]->name() << std::endl;

          std::string * toPartName_p = const_cast<std::string *> (&toParts[i_part]->name());

          std::string toPartName = toParts[i_part]->name();
          if (toParts[i_part]->name() == fromParts[i_part]->name())
            {
              continue;
            }
          std::string fromPartName = toPartName;
          int len = fromPartName.length();
          int clen = breakPattern->getAppendConvertString().length();
          fromPartName.erase(len - clen, clen);

          mesh::Part *fromPart = m_eMesh.getNonConstPart(fromPartName);
          std::string * fromPartName_p = const_cast<std::string *> (&fromPart->name());
          *toPartName_p = fromPartName;
          *fromPartName_p = fromPartName + breakPattern->getAppendOriginalString();

          if (0) std::cout << "tmp  after: fromPartName= " << fromParts[i_part]->name() << " toPartName= " << toParts[i_part]->name() << std::endl;

          if (0)
            std::cout << "tmp P[" << m_eMesh.getRank() << "] fromPartName: " << fromPartName << " part= " << toParts[i_part]->name() 
                      << " old part name = " << fromPart->name()
                      << std::endl;
        }
    }

    /// create a list of nodes from the new nodes that can be deciphered by the UniformRefinerPattern
    /// Returns the 2D array new_sub_entity_nodes[entity_rank][ordinal_of_node_on_sub_dim_entity]
    
    bool UniformRefiner::
    createNewNeededNodeIds(const CellTopologyData * const cell_topo_data, 
                           const Entity& element, vector<NeededEntityType>& needed_entity_ranks, NewSubEntityNodesType& new_sub_entity_nodes)
    {
      EXCEPTWATCH;

      // CHECK
      //const CellTopologyData * const cell_topo_data = get_cell_topology(element);
      //CellTopology cell_topo(cell_topo_data);

      NodeRegistry& nodeRegistry = *m_nodeRegistry;

      const mesh::PairIterRelation elem_nodes = element.relations(Node);

      // CHECK - cache this
      for (unsigned ineed_ent=0; ineed_ent < needed_entity_ranks.size(); ineed_ent++)
        {
          unsigned numSubDimNeededEntities = 0;

          // special case of face in 3d or edge in 2d
          if (needed_entity_ranks[ineed_ent].first == element.entity_rank())
            {
              numSubDimNeededEntities = 1;
            }
          else if (needed_entity_ranks[ineed_ent].first == Edge)
            {
              numSubDimNeededEntities = cell_topo_data->edge_count;
            }
          else if (needed_entity_ranks[ineed_ent].first == Face)
            {
              numSubDimNeededEntities = cell_topo_data->side_count;
            }
          else if (needed_entity_ranks[ineed_ent].first == mesh::Element)
            {
              numSubDimNeededEntities = 1;
            }

          new_sub_entity_nodes[needed_entity_ranks[ineed_ent].first].resize(numSubDimNeededEntities);

          for (unsigned iSubDimOrd = 0; iSubDimOrd < numSubDimNeededEntities; iSubDimOrd++)
            {
              // CHECK
              NodeIdsOnSubDimEntityType& nodeIds_onSE = nodeRegistry.getNewNodesOnSubDimEntity(element, needed_entity_ranks[ineed_ent].first, iSubDimOrd);
              

              if (!nodeIds_onSE[0]) {
                std::cout << "P[" << m_eMesh.getRank() << "] nodeId ## = 0 << " 
                          << " element= " << element
                          << " needed_entity_ranks= " << needed_entity_ranks[ineed_ent].first
                          << " iSubDimOrd = " << iSubDimOrd
                          <<  std::endl;
                throw std::logic_error("UniformRefiner logic error");
              }
              //new_sub_entity_nodes[needed_entity_ranks[ineed_ent].first][iSubDimOrd].resize(
              unsigned num_new_nodes_needed = needed_entity_ranks[ineed_ent].second;
              if (num_new_nodes_needed < 1)
                {
                  //std::cout << "needed_entity_ranks[ineed_ent].second = " << num_new_nodes_needed << std::endl;
                  //throw std::logic_error("needed_entity_ranks[ineed_ent].second");
                  return true;
                }
              new_sub_entity_nodes[needed_entity_ranks[ineed_ent].first][iSubDimOrd].resize(num_new_nodes_needed);
              for (unsigned i_new_node = 0; i_new_node < num_new_nodes_needed; i_new_node++)
                {
                  new_sub_entity_nodes[needed_entity_ranks[ineed_ent].first][iSubDimOrd][i_new_node] = nodeIds_onSE[i_new_node];
                }
            }
        }
      return false;
    }




  } // namespace adapt
} // namespace stk