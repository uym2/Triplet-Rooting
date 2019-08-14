#include "TripletRooting.h"
#include "hdt.h"
#include "hdt_factory.h"

bool TripletRooting::find_optimal_root(){
    // construct HDT for myRef
    myTree->pairAltWorld(myRef);
    if (myTree->isError()) {
        std::cerr << "The two trees do not have the same set of leaves." << std::endl;
        std::cerr << "Aborting." << std::endl;
        return false;
    }

    countChildren(myTree);

    hdt = HDT::constructHDT(myRef, myTree->maxDegree + 1, dummyHDTFactory);
    
    count(myTree);

    // HDT is deleted in count if extracting and contracting!
/*
#ifndef doExtractAndContract
    delete hdt->factory;
#endif
*/    
    this->compute_tA(this->myTree);

    for (int i = 0; i < tripCount->N; i++){
        std::cout << "tA[" << i << "] = " << tripCount->tA[i] << std::endl;
        std::cout << "tI[" << i << "] = " << tripCount->tI[i] << std::endl;
        std::cout << "tO[" << i << "] = " << tripCount->tO[i] << std::endl;
        std::cout << "tR[" << i << "] = " << tripCount->tR[i] << std::endl;
    }

    unsigned int r = myTree->idx;
    this->optimalRoot = this->myTree->children->data; // the first child
    this->optimalTripScore = tripCount->tA[r];
    INTTYPE_REST parent_score = tripCount->tA[r] - tripCount->tI[r];
    //this->downroot(myTree,parent_score);

     
    for(TemplatedLinkedList<RootedTree*> *current = myTree->children; current != NULL; current = current->next) {
        this->downroot(current->data,parent_score);
    } 
    return true;
}

void TripletRooting::downroot(RootedTree *t, INTTYPE_REST parent_score){
    unsigned int u = t->idx;
    
    for(TemplatedLinkedList<RootedTree*> *current = t->children; current != NULL; current = current->next) {
        unsigned int v = current->data->idx;
        INTTYPE_REST current_score = parent_score - tripCount->tI[u] + tripCount->tO[v] + tripCount->tR[v];
        std::cout << "u = " << u << "; v = " << v << endl;
        current->data->print_leaves();
        std::cout << "current_score = " << current_score << std::endl;
        if (current_score > this->optimalTripScore){
            this->optimalTripScore = current_score;
            this->optimalRoot = current->data;
        }
        INTTYPE_REST v_parent_score = current_score - tripCount->tR[v]; 
        this->downroot(current->data,v_parent_score);
    }   
}

TripletRooting::TripletRooting(RootedTree *ref, RootedTree *tree){
    this->myRef = ref;
    this->myTree = tree;
    // TODO: if there is multifurcating at the root of myTree, reroot it on one of the root's children

    this->hdt = NULL;
    unsigned int N = this->myTree->set_all_idx(0);
    tripCount = new TripletCounter(N);
    dummyHDTFactory = new HDTFactory(myTree->maxDegree+1);
}

TripletRooting::~TripletRooting(){
    //delete [] tripCount;
    //delete dummyHDTFactory;
}

void TripletRooting::update_tI(unsigned int nodeIdx){
        this->tripCount->tI[nodeIdx] = this->hdt->getResolvedTriplets(0) +  this->hdt->getUnresolvedTriplets(0);
}

void TripletRooting::update_tO(unsigned int nodeIdx, unsigned int color){
        this->tripCount->tO[nodeIdx] = this->hdt->getResolvedTriplets(color) + this->hdt->getUnresolvedTriplets(color);
}

void TripletRooting::update_tR(unsigned int nodeIdx){
    this->tripCount->tR[nodeIdx] = this->hdt->getResolvedTriplets_root();
}


/*
void TripletRooting::updateCounters(unsigned int nodeIdx, unsigned int color){
    if (color == 0) {
        // update tI
        this->tripCount->tI[nodeIdx] = this->hdt->getResolvedTriplets(0) + this->hdt->getUnresolvedTriplets(0);
    }
    else { 
        // update tO
        this->tripCount->tO[nodeIdx] = this->hdt->getResolvedTriplets(color) + this->hdt->getUnresolvedTriplets(color);
    }
    // update tR
    this->tripCount->tR[nodeIdx] = this->hdt->getResolvedTriplets_root();
}*/

void TripletRooting::compute_tA(RootedTree *v){
    unsigned int acc = this->tripCount->tI[v->idx];
    for(TemplatedLinkedList<RootedTree*> *current = v->children; current != NULL; current = current->next) {
        this->compute_tA(current->data);
        acc += this->tripCount->tA[current->data->idx];
    }
    this->tripCount->tA[v->idx] = acc;    
}


void TripletRooting::count(RootedTree *v) {
  //if (v->isLeaf() || v->n <= 2) {
  if (v->isLeaf()) {    
    hdt->updateCounters();
    //std::cout << "ResolvedTriplets_root = " << this->hdt->getResolvedTriplets_root() << std::endl;
    //updateCounters(v->idx,1);
    update_tR(v->idx);

    // This will make sure the entire subtree has color 0!
    v->colorSubtree(0);

/*    
#ifdef doExtractAndContract
    delete hdt->factory;
#endif
*/    
    return;
  }

  // v is not a leaf!
  // Find largest subtree
  TemplatedLinkedList<RootedTree*> *largest = v->children;
  int largestN = largest->data->n;
  TemplatedLinkedList<RootedTree*> *beforeLargest = NULL;
  TemplatedLinkedList<RootedTree*> *prev = v->children;
  for(TemplatedLinkedList<RootedTree*> *current = v->children->next; current != NULL; current = current->next) {
    if (current->data->n > largestN) {
      largest = current;
      beforeLargest = prev;
      largestN = largest->data->n;
    }
    prev = current;
  }
  if (beforeLargest != NULL) {
    beforeLargest->next = largest->next;
    largest->next = v->children;
    v->children = largest;
  }
  
  // Color i'th subtree (i > 1) with color i
  int c = 2;
  for(TemplatedLinkedList<RootedTree*> *current = v->children->next; current != NULL; current = current->next) {
    current->data->colorSubtree(c);
    c++;
  }

  // Update counters in the HDT
  hdt->updateCounters();
  //updateCounters(v->idx,0); // triplets inside v
  update_tI(v->idx);
  update_tR(v->idx);
  if (v != this->myTree) // v is not the root
  {
      // compute triplets outside of each child of v
      int c = 1;
      for(TemplatedLinkedList<RootedTree*> *current = v->children; current != NULL; current = current->next) {
          //updateCounters(current->data->idx,c);
          std::cout << "Updating tO of node index " << current->data->idx << " which was colored " << c << std::endl;
          std::cout << "Subtree: ";
          current->data->print_leaves();
          std::cout << std::endl;
          std::cout << "Before updating: tO = " << tripCount->tO[current->data->idx] << std::endl;
          update_tO(current->data->idx,c);
          std::cout << "After updating: tO = " << tripCount->tO[current->data->idx] << std::endl;
          c++;
      }      
  }

/*  
#ifdef doExtractAndContract
  // Extract
  RootedTree** extractedVersions = new RootedTree*[v->numChildren - 1];
  c = 0;
  for(TemplatedLinkedList<RootedTree*> *current = v->children->next; current != NULL; current = current->next) {
    if (current->data->isLeaf() || current->data->n <= 2) {
      extractedVersions[c] = NULL;
    } else {
      current->data->markHDTAlternative();
      RootedTree *extractedT2 = hdt->extractAndGoBack(myTree->factory);
      extractedVersions[c] = extractedT2->contract();
      delete extractedT2->factory;
    }
    c++; // Weee :)
  }
#endif
*/

  // Color to 0
  for(TemplatedLinkedList<RootedTree*> *current = v->children->next; current != NULL; current = current->next) {
    current->data->colorSubtree(0);
  }

  // Contract and recurse on 1st child
  RootedTree *firstChild = v->children->data;
  //if (firstChild->isLeaf() || firstChild->n <= 2) {
  if (firstChild->isLeaf()) {    
    // Do "nothing" (except clean up and possibly color!)
/*
#ifdef doExtractAndContract
    // Notice no recoloring here... It's not neccesary as it is extracted and contracted away,
    // and will actually cause an error if called with firstChild->colorSubtree(0) as myRef is linked
    // to a non-existing hdt (as we just deleted it) (we could wait with deleting it, but as we don't need the coloring why bother)
    delete hdt->factory;
#else */
    hdt->updateCounters();
    //updateCounters(firstChild->idx,1);
    update_tR(firstChild->idx);
    firstChild->colorSubtree(0);
/*
#endif */
  } else {
/*      
#ifdef doExtractAndContract
    bool hdtTooBig = firstChild->n * CONTRACT_MAX_EXTRA_SIZE < hdt->leafCount();
    if (hdtTooBig) {
      HDT *newHDT;
      
      firstChild->markHDTAlternative();
      RootedTree *extractedT2 = hdt->extractAndGoBack(myTree->factory);
      RootedTree *contractedT2 = extractedT2->contract();
      delete extractedT2->factory;
      newHDT = HDT::constructHDT(contractedT2, myTree->maxDegree, dummyHDTFactory, true);
      delete contractedT2->factory;
      delete hdt->factory;
      hdt = newHDT;
    }
#endif */
    count(firstChild);
    // HDT is deleted in recursive call!
  }
  
  // Color 1 and recurse
  c = 0;
  for(TemplatedLinkedList<RootedTree*> *current = v->children->next; current != NULL; current = current->next) {
  //if (!current->data->isLeaf() && current->data->n > 2)  {
  if (!current->data->isLeaf()){    
/*
#ifdef doExtractAndContract
      hdt = HDT::constructHDT(extractedVersions[c], myTree->maxDegree, dummyHDTFactory, true);
      delete extractedVersions[c]->factory;
#endif */
      
      current->data->colorSubtree(1);
      
      count(current->data);
    } else {
        current->data->colorSubtree(1);
        hdt->updateCounters();
        //updateCounters(current->data->idx,1);
        update_tR(current->data->idx);
        current->data->colorSubtree(0);
    }
    
    c++; // Weee :)
    // HDT is deleted on recursive calls!
  }
/*  
#ifdef doExtractAndContract
  delete[] extractedVersions;
#endif */
}

void TripletRooting::countChildren(RootedTree *t) {
  if (t->isLeaf()) {
    t->n = 1;
    return;
  }
  
  int nSum = 0;
  for(TemplatedLinkedList<RootedTree*> *i = t->children; i != NULL; i = i->next) {
    RootedTree *childI = i->data;
    countChildren(childI);
    nSum += childI->n;
  }
  t->n = nSum;
}
