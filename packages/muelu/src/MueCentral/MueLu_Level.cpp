#include <Teuchos_TabularOutputter.hpp>

#include "MueLu_Level.hpp"

#include "MueLu_FactoryManagerBase.hpp"

#undef HEAVY_DEBUG_OUTPUT

namespace MueLu {

  Level::Level() : levelID_(-1) { }

  Level::Level(RCP<FactoryManagerBase> & factoryManager) : levelID_(-1), factoryManager_(factoryManager) { }

  RCP<Level> Level::Build() {
    RCP<Level> newLevel = rcp( new Level() );

    // Copy 'keep' status of variables
    // TODO: this only concerns needs_. so a function in Needs class should be provided to do that!
    // TODO: how can i move this to Needs? maybe we need a new constructor for Level which gets a
    // Needs object...
    typedef std::vector<std::string> KeyList;
      
    KeyList ekeys = needs_.RequestedKeys();
    for (KeyList::iterator it = ekeys.begin(); it != ekeys.end(); it++) {
      std::vector<const MueLu::FactoryBase*> ehandles = needs_.RequestedFactories(*it);
      for (std::vector<const MueLu::FactoryBase*>::iterator kt = ehandles.begin(); kt != ehandles.end(); kt++) {
        const std::string & ename = *it;
        const MueLu::FactoryBase* fac = *kt;
        if (IsKept(ename, fac)) {
          if (fac == NULL) 
            newLevel->Keep(ename);
          else 
            newLevel->Keep(ename, fac);
        }
      }
    }
      
    return newLevel;
  }

  Level::~Level() {}

  int Level::GetLevelID() const { return levelID_; }

  void Level::SetLevelID(int levelID) {
    if (levelID_ != -1 && levelID_ != levelID)
      GetOStream(Warnings1, 0) << "Warning: Level::SetLevelID(): Changing an already defined LevelID (previousID=" << levelID_ << "newID=" << levelID << std::endl;

    levelID_ = levelID;
  }

  RCP<Level> & Level::GetPreviousLevel() { return previousLevel_; }

  void Level::SetPreviousLevel(const RCP<Level> & previousLevel) {
    if (previousLevel_ != Teuchos::null && previousLevel_ != previousLevel)
      GetOStream(Warnings1, 0) << "Warning: Level::SetPreviousLevel(): PreviousLevel was already defined" << std::endl;
    
    previousLevel_ = previousLevel;
  }
 
  void Level::SetFactoryManager(const RCP<const FactoryManagerBase> & factoryManager) {
    factoryManager_ = factoryManager;
  }

  void Level::Keep(const std::string& ename, const FactoryBase* factory) {
    TEUCHOS_TEST_FOR_EXCEPTION(factory==NULL, Exceptions::RuntimeError, "MueLu::Level::Keep(): Factory for " << ename << " must not be NULL");

    // mark (ename,factory) to be kept
    needs_.Keep(ename, factory);
    //needs_.Keep(ename, GetFactory(ename, factory));
  }

  bool Level::IsKept(const std::string& ename, const FactoryBase* factory) const {
    return needs_.IsKept(ename, GetFactory(ename, factory));
  }

  void Level::Delete(const std::string& ename, const FactoryBase* factory) {
    needs_.Keep(ename, GetFactory(ename, factory), false);
  }

  void Level::Request(const FactoryBase& factory) {
    RequestMode prev = requestMode_;
    requestMode_ = REQUEST;
    factory.CallDeclareInput(*this);
    requestMode_ = prev;
  }

  void Level::Release(const FactoryBase& factory) {
    RequestMode prev = requestMode_;
    requestMode_ = RELEASE;
    factory.CallDeclareInput(*this);
    requestMode_ = prev;
  }

  void Level::DeclareInput(const std::string& ename, const FactoryBase* factory, const FactoryBase* requestedBy) {
    if (requestMode_ == REQUEST) {
      // old stuff
      Request(ename, factory, requestedBy);
      // new stuff
      //const FactoryBase* fac = GetFactory(ename, factory);
      //needs_.Request(ename, fac, requestedBy);
      //std::cout << requestedBy << " request " << ename << " gen by " << fac << std::endl;
    }
    else if (requestMode_ == RELEASE) {

      // new stuff
      /*const FactoryBase* fac = GetFactory(ename, factory);
      if(needs_.IsRequestedBy(fac, ename, requestedBy)) {
        //std::cout << "variable (" << ename << "," << fac << ") has been requested by " << requestedBy << " " << needs_.NumRequestsBy(fac, ename, requestedBy) << " times and is now released." << std::endl;
        needs_.Release(ename,fac,requestedBy);
        std::cout << "Release " << ename << " gen by " << fac << " requested by " << requestedBy << std::endl;
        Release(ename, factory);  // do release :-)
      }
      else {
        std::cout << "variable (" << ename << "," << fac << ") has NOT been requested by " << requestedBy << " (isKept=" << IsKept(ename, fac) << ")"<< std::endl;
      }*/

      // TODO move me
      // only call Release, if (ename,factory) has also been requested by requestedFac before!
      Release(ename, factory, requestedBy);
    }
    else
      TEUCHOS_TEST_FOR_EXCEPTION(true, Exceptions::RuntimeError, "MueLu::Level::DeclareInput(): requestMode_ undefined.");
  }

  void Level::DeclareDependencies(const FactoryBase* factory, bool bRequestOnly, bool bReleaseOnly) { //TODO: replace bReleaseOnly, bReleaseOnly by one RequestMode enum
    if (bRequestOnly && bReleaseOnly)
      TEUCHOS_TEST_FOR_EXCEPTION(true, Exceptions::RuntimeError, "MueLu::Level::DeclareDependencies(): Both bRequestOnly and bReleaseOnly set to true makes no sense.");

    if (requestMode_ == REQUEST) {

      if (bReleaseOnly == false) Request(*factory);

    } else if (requestMode_ == RELEASE) {

      if (bRequestOnly == false) Release(*factory);

    } else TEUCHOS_TEST_FOR_EXCEPTION(true, Exceptions::RuntimeError, "MueLu::Level::DeclareDependencies(): requestMode_ undefined.");
  }

  void Level::Request(const std::string& ename, const FactoryBase* factory, const FactoryBase* requestedBy) {
    const FactoryBase* fac = GetFactory(ename, factory);

    // call Request for factory fac only if
    // the factory has not been requested before and no data has
    // been generated by fact (independent of ename)
    if ((needs_.IsAvailableFactory(fac) == false && needs_.IsRequestedFactory(fac) == false)) {
      Request(*fac);
    }

    // request data 'ename' generated by 'fac'
    needs_.Request(ename, fac);

    // new stuff
    needs_.Request(ename, fac, requestedBy);
    std::cout << requestedBy << " request " << ename << " gen by " << fac << std::endl;

  }

  void Level::Release(const std::string& ename, const FactoryBase* factory, const FactoryBase* requestedBy) {
    const FactoryBase* fac = GetFactory(ename, factory);

#if 1
    if(IsRequested(ename,fac) == true) {

      // new stuff******
      if(needs_.IsRequestedBy(fac, ename, requestedBy)) {
        //std::cout << "variable (" << ename << "," << fac << ") has been requested by " << requestedBy << " " << needs_.NumRequestsBy(fac, ename, requestedBy) << " times and is now released." << std::endl;
        needs_.Release(ename,fac,requestedBy);
        std::cout << "Release " << ename << " gen by " << fac << " requested by " << requestedBy << std::endl;

        // begin old stuff
        // data has been requested but never built
        // can we release the dependencies of fac safely?
        int cnt = needs_.CountRequestedFactory(fac);
        if(cnt == 1) {
          // factory is only generating factory of current variable (ename,factory)
          // Release(fac) can be called safely
          Release(*fac);
        }
        needs_.Release(ename, fac);
        // end old stuff
      }
      else {
        std::cout << "variable (" << ename << "," << fac << ") has NOT been requested by " << requestedBy << " (isKept=" << IsKept(ename, fac) << ")"<< std::endl;
      }
      //*****************
    }
#else
    if(needs_.IsRequestedBy(fac, ename, requestedBy)) {
      //std::cout << "variable (" << ename << "," << fac << ") has been requested by " << requestedBy << " " << needs_.NumRequestsBy(fac, ename, requestedBy) << " times and is now released." << std::endl;
      needs_.Release(ename,fac,requestedBy);
      std::cout << "Release " << ename << " gen by " << fac << " requested by " << requestedBy << std::endl;

      // begin old stuff
      // data has been requested but never built
      // can we release the dependencies of fac safely?

      RCP<Teuchos::FancyOStream> out = Teuchos::fancyOStream(Teuchos::rcpFromRef(std::cout));
      print(*out);

      int cnt = needs_.CountRequestedFactory(fac);
      if(cnt == 1 /*&& needs_.NumRequests(ename, fac) == 1*/) {
        // factory is only generating factory of current variable (ename,factory)
        // Release(fac) can be called safely
        std::cout << "Level::Release(" << factory << ")" << std::endl;
        Release(*fac);
      }
      needs_.Release(ename, fac);
      // end old stuff
    }
    /*else if(IsRequested(ename,fac) == true) {
      // data has been requested but never built
      // can we release the dependencies of fac safely?
      int cnt = needs_.CountRequestedFactory(fac);
      if(cnt == 1) {
        // factory is only generating factory of current variable (ename,factory)
        // Release(fac) can be called safely
        Release(*fac);
      }
      needs_.Release(ename, fac);
    }*/
#endif
    else {
      // check for error!
      TEUCHOS_TEST_FOR_EXCEPTION(IsAvailable(ename,fac) == false && IsRequested(ename,fac) == false && IsKept(ename,fac) == true, Exceptions::RuntimeError,
                                 "MueLu::Level::Release(): Data not requested and not available, but kept. Call of Release does not make sense. You should check this!");
      TEUCHOS_TEST_FOR_EXCEPTION(IsAvailable(ename,fac) == true && IsRequested(ename,fac) == false && IsKept(ename,fac) == false, Exceptions::RuntimeError,
                                 "MueLu::Level::Release(): Strange! Data is available but not requested and not kept??");
    }
  }

  bool Level::IsAvailable(const std::string & ename, const FactoryBase* factory) {
    return needs_.IsAvailable(ename, GetFactory(ename, factory));
  }

  bool Level::IsRequested(const std::string & ename, const FactoryBase* factory) {
    return needs_.IsRequested(ename, GetFactory(ename, factory));
  }

  void Level::print(Teuchos::FancyOStream &out, const VerbLevel verbLevel) const {
    Teuchos::TabularOutputter outputter(out);
    outputter.pushFieldSpec("name",               Teuchos::TabularOutputter::STRING, Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 32);
    outputter.pushFieldSpec("gen. factory addr.", Teuchos::TabularOutputter::STRING, Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 18);
    outputter.pushFieldSpec("req",                Teuchos::TabularOutputter::INT,    Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 3);
    outputter.pushFieldSpec("keep",               Teuchos::TabularOutputter::STRING,    Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 5);
    outputter.pushFieldSpec("type",               Teuchos::TabularOutputter::STRING, Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 10);
    outputter.pushFieldSpec("data",               Teuchos::TabularOutputter::STRING, Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 20);
    outputter.outputHeader();

    std::vector<std::string> ekeys = needs_.RequestedKeys();
    for (std::vector<std::string>::iterator it = ekeys.begin(); it != ekeys.end(); it++) {
      std::vector<const MueLu::FactoryBase*> ehandles = needs_.RequestedFactories(*it);
      for (std::vector<const MueLu::FactoryBase*>::iterator kt = ehandles.begin(); kt != ehandles.end(); kt++) {
        outputter.outputField(*it);   // variable name
        outputter.outputField(*kt);   // factory ptr

        int reqcount = needs_.NumRequests(*it, *kt); // request counter
        outputter.outputField(reqcount);
        if (needs_.IsKept(*it, *kt)) {
          outputter.outputField("true");
        }
        else
          {
            outputter.outputField("false");
          }

        if (needs_.IsAvailable(*it, *kt)) {
          std::string strType = needs_.GetType(*it, *kt); // Variable type
          if (strType.find("Xpetra::Operator") != std::string::npos) {
            outputter.outputField("Operator" );
            outputter.outputField(" ");
          } else if (strType.find("Xpetra::MultiVector") != std::string::npos) {
            outputter.outputField("Vector");
            outputter.outputField("");
          } else if (strType.find("MueLu::SmootherBase") != std::string::npos) {
            outputter.outputField("SmootherBase");
            outputter.outputField("");
          } else if (strType == "int") {
            outputter.outputField(strType);
            int data = needs_.Get<int>(*it, *kt);
            outputter.outputField(data);
          } else if (strType == "double") {
            outputter.outputField(strType);
            double data = needs_.Get<double>(*it, *kt);
            outputter.outputField(data);
          } else if (strType == "string") {
            outputter.outputField(strType);
            std::string data = needs_.Get<std::string>(*it, *kt);
            outputter.outputField(data);
          } else {
            outputter.outputField(strType);
            outputter.outputField("");
          }
        } else {
          outputter.outputField("unknown");
          outputter.outputField("not available");
        }


        outputter.nextRow();
      }
    }

  }

  Level::Level(const Level& source) { }

  const FactoryBase* Level::GetFactory(const std::string& varname, const FactoryBase* factory) const {
    if (factory == NULL) {
      TEUCHOS_TEST_FOR_EXCEPTION(factoryManager_ == null, Exceptions::RuntimeError, "MueLu::Level::GetFactory(): no FactoryManager");
      const FactoryBase* fac = factoryManager_->GetFactory(varname).get();
      TEUCHOS_TEST_FOR_EXCEPTION(fac == NULL, Exceptions::RuntimeError, "MueLu::Level::GetFactory(): Default factory cannot be NULL");
      return fac;
    } else {
      return factory;
    }
  }

  Level::RequestMode Level::requestMode_ = UNDEF;

} //namespace MueLu

//TODO: Caps should not matter
