/* Copyright (C) 2016
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMULATION_DATA_MODELCACHES
#define BLACKMISC_SIMULATION_DATA_MODELCACHES

#include "blackmisc/simulation/aircraftmodelinterfaces.h"
#include "blackmisc/simulation/aircraftmodellist.h"
#include "blackmisc/simulation/simulatorinfo.h"
#include "blackmisc/applicationinfo.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/datacache.h"
#include "blackmisc/blackmiscexport.h"

#include <QDateTime>
#include <QObject>

namespace BlackMisc
{
    namespace Simulation
    {
        namespace Data
        {
            //! Trait for model cache
            struct TModelCache : public TDataTrait<CAircraftModelList>
            {
                //! Defer loading
                static constexpr bool isDeferred() { return true; }
            };

            //! \name Caches for own models on disk, loaded by IAircraftModelLoader
            //! @{

            //! XPlane
            struct TModelCacheXP : public TModelCache
            {
                //! Key in data cache
                static const char *key() { return "modelcachexp"; }
            };

            //! FSX
            struct TModelCacheFsx : public TModelCache
            {
                //! Key in data cache
                static const char *key() { return "modelcachefsx"; }
            };

            //! FS9
            struct TModelCacheFs9 : public TModelCache
            {
                //! Key in data cache
                static const char *key() { return "modelcachefs9"; }
            };

            //! P3D
            struct TModelCacheP3D : public TModelCache
            {
                //! Key in data cache
                static const char *key() { return "modelcachep3d"; }
            };

            //! Last selection
            struct TModelCacheLastSelection : public TDataTrait<CSimulatorInfo>
            {
                //! First load is synchronous
                static constexpr bool isPinned() { return true; }

                //! Default simulator
                static const CSimulatorInfo &defaultValue() { return CSimulatorInfo::guessDefaultSimulator(); }

                //! Key
                static const char *key() { return "modelcachelastselection"; }
            };
            //! @}

            //! \name Caches for choosen model sets
            //! @{

            //! XPlane
            struct TModelSetCacheXP : public TModelCache
            {
                //! Key in data cache
                static const char *key() { return "modelsetxp"; }
            };

            //! FSX
            struct TModelSetCacheFsx : public TModelCache
            {
                //! Key in data cache
                static const char *key() { return "modelsetfsx"; }
            };

            //! FS9
            struct TModelSetCacheFs9 : public TModelCache
            {
                //! Key in data cache
                static const char *key() { return "modelsetfs9"; }
            };

            //! P3D
            struct TModelSetCacheP3D : public TModelCache
            {
                //! Key in data cache
                static const char *key() { return "modelsetp3d"; }
            };

            //! Last selection
            struct TSimulatorLastSelection : public BlackMisc::TDataTrait<CSimulatorInfo>
            {
                //! First load is synchronous
                static constexpr bool isPinned() { return true; }

                //! Default simulator
                static const CSimulatorInfo &defaultValue() { return CSimulatorInfo::guessDefaultSimulator(); }

                //! Key
                static const char *key() { return "simulatorlastselection"; }
            };

            //! Last selections
            struct TSimulatorLastSelections : public BlackMisc::TDataTrait<CSimulatorInfo>
            {
                //! First load is synchronous
                static constexpr bool isPinned() { return true; }

                //! Default simulator
                static const CSimulatorInfo &defaultValue() { return CSimulatorInfo::guessDefaultSimulator(); }

                //! Key
                static const char *key() { return "simulatorlastselections"; }
            };
            //! @}

            //! Trait for vPilot derived models
            struct TVPilotAircraftModels : public TModelCache
            {
                //! Key in data cache
                static const char *key() { return "vpilot/models"; }
            };

            //! Cache for multiple simulators specified by CSimulatorInfo
            class BLACKMISC_EXPORT IMultiSimulatorModelCaches :
                public QObject,
                public IModelsForSimulatorSetable,
                public IModelsForSimulatorUpdatable
            {
                Q_OBJECT
                Q_INTERFACES(BlackMisc::Simulation::IModelsForSimulatorSetable)
                Q_INTERFACES(BlackMisc::Simulation::IModelsForSimulatorUpdatable)

            public:
                //! Models for simulator
                //! \threadsafe
                virtual CAircraftModelList getCachedModels(const CSimulatorInfo &simulator) const = 0;

                //! Models
                CAircraftModelList getSynchronizedCachedModels(const CSimulatorInfo &simulator);

                //! Count of models for simulator
                int getCachedModelsCount(const CSimulatorInfo &simulator) const;

                //! Get filename for simulator cache file
                virtual QString getFilename(const CSimulatorInfo &simulator) const = 0;

                //! Has the other version the file?
                bool hasOtherVersionFile(const BlackMisc::CApplicationInfo &info, const CSimulatorInfo &simulator) const;

                //! Simulators of given other versionwhich have a cache file
                CSimulatorInfo otherVersionSimulatorsWithFile(const BlackMisc::CApplicationInfo &info) const;

                //! All file names
                virtual QStringList getAllFilenames() const;

                //! Simulator which uses cache with filename
                CSimulatorInfo getSimulatorForFilename(const QString &filename) const;

                //! Cache timestamp
                //! \threadsafe
                virtual QDateTime getCacheTimestamp(const CSimulatorInfo &simulator) const = 0;

                //! Set cache timestamp
                virtual CStatusMessage setCacheTimestamp(const QDateTime &ts, const CSimulatorInfo &simulator) = 0;

                //! Cache saved?
                //! \threadsafe
                virtual bool isSaved(const CSimulatorInfo &simulator) const = 0;

                //! Initialized caches for which simulator?
                //! \threadsafe
                CSimulatorInfo simulatorsWithInitializedCache() const;

                //! Timestamp
                QDateTime getSynchronizedTimestamp(const CSimulatorInfo &simulator);

                //! Set cached models
                //! \threadsafe
                virtual CStatusMessage setCachedModels(const CAircraftModelList &models, const CSimulatorInfo &simulator) = 0;

                //! Clear cached models
                //! \threadsafe
                virtual CStatusMessage clearCachedModels(const CSimulatorInfo &simulator);

                //! Synchronize for given simulator
                virtual void synchronizeCache(const CSimulatorInfo &simulator) = 0;

                //! Admit the cache for given simulator
                //! \threadsafe
                virtual void admitCache(const CSimulatorInfo &simulator) = 0;

                //! \copydoc IModelsForSimulatorSetable::setModels
                virtual void setModelsForSimulator(const CAircraftModelList &models, const CSimulatorInfo &simulator) override;

                //! \copydoc IModelsForSimulatorUpdatable::updateModels
                virtual int updateModelsForSimulator(const CAircraftModelList &models, const CSimulatorInfo &simulator) override;

                //! Info string about models in cache
                QString getInfoString() const;

                //! Info string without XPlane (FSX,P3D, FS9)
                QString getInfoStringFsFamily() const;

                //! Cache count and timestamp
                QString getCacheCountAndTimestamp(const CSimulatorInfo &simulator) const;

                //! Graceful shutdown
                virtual void gracefulShutdown();

                //! Descriptive text
                virtual QString getDescription() const = 0;

            signals:
                //! Cache has been changed
                void cacheChanged(const CSimulatorInfo &simulator);

            protected:
                //! Construtor
                IMultiSimulatorModelCaches(QObject *parent = nullptr) : QObject(parent)
                { }

                //! \name Cache has been changed. This will only detect changes elsewhere, owned caches will not signal local changes
                //! @{
                void changedFsx() { emitCacheChanged(CSimulatorInfo::fsx()); }
                void changedFs9() { emitCacheChanged(CSimulatorInfo::fs9()); }
                void changedP3D() { emitCacheChanged(CSimulatorInfo::p3d()); }
                void changedXP()  { emitCacheChanged(CSimulatorInfo::xplane()); }
                //! @}

                //! Emit cacheChanged() utility function (allows breakpoint)
                void emitCacheChanged(const CSimulatorInfo &simulator);
            };

            //! Bundle of caches for all simulators
            //! \remark remembers its last simulator selection
            class BLACKMISC_EXPORT CModelCaches : public IMultiSimulatorModelCaches
            {
                Q_OBJECT

            public:
                //! Construtor
                CModelCaches(bool synchronizeCache, QObject *parent = nullptr);

                //! Log categories
                static const BlackMisc::CLogCategoryList &getLogCategories();

                //! \name Interface implementations
                //! @{
                virtual CAircraftModelList getCachedModels(const CSimulatorInfo &simulator) const override;
                virtual CStatusMessage setCachedModels(const CAircraftModelList &models, const CSimulatorInfo &simulator) override;
                virtual QDateTime getCacheTimestamp(const CSimulatorInfo &simulator) const override;
                virtual CStatusMessage setCacheTimestamp(const QDateTime &ts, const CSimulatorInfo &simulator) override;
                virtual void synchronizeCache(const CSimulatorInfo &simulator) override;
                virtual void admitCache(const CSimulatorInfo &simulator) override;
                virtual QString getFilename(const CSimulatorInfo &simulator) const override;
                virtual bool isSaved(const CSimulatorInfo &simulator) const override;
                virtual QString getDescription() const override { return "Model caches"; }
                //! @}

            private:
                CData<Data::TModelCacheFsx> m_modelCacheFsx {this, &CModelCaches::changedFsx }; //!< FSX cache
                CData<Data::TModelCacheFs9> m_modelCacheFs9 {this, &CModelCaches::changedFs9 }; //!< FS9 cache
                CData<Data::TModelCacheP3D> m_modelCacheP3D {this, &CModelCaches::changedP3D }; //!< P3D cache
                CData<Data::TModelCacheXP>  m_modelCacheXP  {this, &CModelCaches::changedXP };  //!< XP cache

                //! Non virtual version (can be used in ctor)
                void synchronizeCacheImpl(const CSimulatorInfo &simulator);

                //! Non virtual version (can be used in ctor)
                //! \threadsafe
                void admitCacheImpl(const CSimulatorInfo &simulator);
            };

            //! Bundle of caches for model sets of all simulators
            //! \remark remembers its last simulator selection
            class BLACKMISC_EXPORT CModelSetCaches : public IMultiSimulatorModelCaches
            {
                Q_OBJECT

            public:
                //! Construtor
                CModelSetCaches(bool synchronizeCache, QObject *parent = nullptr);

                //! Log categories
                static const BlackMisc::CLogCategoryList &getLogCategories();

                //! \name Interface implementations
                //! @{
                virtual CAircraftModelList getCachedModels(const CSimulatorInfo &simulator) const override;
                virtual CStatusMessage setCachedModels(const CAircraftModelList &models, const CSimulatorInfo &simulator) override;
                virtual QDateTime getCacheTimestamp(const CSimulatorInfo &simulator) const override;
                virtual CStatusMessage setCacheTimestamp(const QDateTime &ts, const CSimulatorInfo &simulator) override;
                virtual void synchronizeCache(const CSimulatorInfo &simulator) override;
                virtual void admitCache(const CSimulatorInfo &simulator) override;
                virtual QString getFilename(const CSimulatorInfo &simulator) const override;
                virtual bool isSaved(const CSimulatorInfo &simulator) const override;
                virtual QString getDescription() const override { return "Model sets"; }
                //! @}

            private:
                CData<Data::TModelSetCacheFsx> m_modelCacheFsx {this, &CModelSetCaches::changedFsx };  //!< FSX cache
                CData<Data::TModelSetCacheFs9> m_modelCacheFs9 {this, &CModelSetCaches::changedFs9};   //!< FS9 cache
                CData<Data::TModelSetCacheP3D> m_modelCacheP3D {this, &CModelSetCaches::changedP3D };  //!< P3D cache
                CData<Data::TModelSetCacheXP>  m_modelCacheXP  {this, &CModelSetCaches::changedXP };   //!< XP cache

                //! Non virtual version (can be used in ctor)
                void synchronizeCacheImpl(const CSimulatorInfo &simulator);

                //! Non virtual version (can be used in ctor)
                //! \threadsafe
                void admitCacheImpl(const CSimulatorInfo &simulator);
            };

            //! One central instance of the caches
            class BLACKMISC_EXPORT CCentralMultiSimulatorModelCachesProvider : public IMultiSimulatorModelCaches
            {
                Q_OBJECT
                Q_INTERFACES(BlackMisc::Simulation::IModelsForSimulatorSetable)
                Q_INTERFACES(BlackMisc::Simulation::IModelsForSimulatorUpdatable)

            public:
                //! Central instance
                static CCentralMultiSimulatorModelCachesProvider &modelCachesInstance();

                //! \name Interface implementations
                //! @{
                virtual CAircraftModelList getCachedModels(const CSimulatorInfo &simulator) const override;
                virtual CStatusMessage setCachedModels(const CAircraftModelList &models, const CSimulatorInfo &simulator) override;
                virtual QDateTime getCacheTimestamp(const CSimulatorInfo &simulator) const override;
                virtual CStatusMessage setCacheTimestamp(const QDateTime &ts, const CSimulatorInfo &simulator) override;
                virtual void synchronizeCache(const CSimulatorInfo &simulator) override;
                virtual void admitCache(const CSimulatorInfo &simulator) override;
                virtual QString getFilename(const CSimulatorInfo &simulator) const override;
                virtual bool isSaved(const CSimulatorInfo &simulator) const override;
                virtual QString getDescription() const override;
                //! @}

            protected:
                //! Ctor
                CCentralMultiSimulatorModelCachesProvider(QObject *parent = nullptr);

            private:
                CModelCaches m_caches { this }; //!< the caches

                //! Singleton caches
                CModelCaches &instanceCaches() { return this->isInstance() ? m_caches : modelCachesInstance().m_caches; }

                //! Singleton caches
                const CModelCaches &instanceCaches() const { return this->isInstance() ? m_caches : modelCachesInstance().m_caches; }

                //! Is this object the central instance?
                bool isInstance() const { return this == &modelCachesInstance(); }
            };
        } // ns
    } // ns
} // ns

#endif // guard
