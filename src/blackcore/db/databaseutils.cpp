/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/application.h"
#include "databaseutils.h"
#include "blackcore/webdataservices.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/fileutils.h"

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Simulation;

namespace BlackCore
{
    namespace Db
    {
        const CLogCategoryList &CDatabaseUtils::getLogCategories()
        {
            static const BlackMisc::CLogCategoryList cats { CLogCategory::modelCache(), CLogCategory::modelSetCache() };
            return cats;
        }

        CAircraftModel CDatabaseUtils::consolidateOwnAircraftModelWithDbData(const CAircraftModel &model, bool force, bool *modified)
        {
            bool myModified = false;
            CAircraftModel ownModel = CDatabaseUtils::consolidateModelWithDbData(model, force, &myModified);
            // special case here, as we have some specific values for a local model

            if (myModified)
            {
                ownModel.updateMissingParts(model);
                ownModel.setFileName(model.getFileName());
                myModified = true;
            }
            if (ownModel.getModelType() != CAircraftModel::TypeOwnSimulatorModel)
            {
                ownModel.setModelType(CAircraftModel::TypeOwnSimulatorModel);
                myModified = true;
            }
            if (modified) { *modified = myModified; }
            return ownModel;
        }

        CAircraftModel CDatabaseUtils::consolidateModelWithDbData(const CAircraftModel &model, bool force, bool *modified)
        {
            if (modified) { *modified = false; }
            if (!model.hasModelString()) { return model; }
            if (!hasDbAircraftData()) { return model; }

            CAircraftModel dbModel(sApp->getWebDataServices()->getModelForModelString(model.getModelString()));
            return CDatabaseUtils::consolidateModelWithDbData(model, dbModel, force, modified);
        }

        CAircraftModel CDatabaseUtils::consolidateModelWithDbData(const CAircraftModel &model, const CAircraftModel &dbModel, bool force, bool *modified)
        {
            Q_ASSERT_X(sApp, Q_FUNC_INFO, "Missing application object");
            Q_ASSERT_X(sApp->hasWebDataServices(), Q_FUNC_INFO, "No web services");

            if (modified) { *modified = false; }
            if (!model.hasModelString()) { return model; }
            if (!force && model.hasValidDbKey()) { return model; }
            const int distributorOrder = model.getDistributorOrder(); // later restore that order

            if (dbModel.isLoadedFromDb())
            {
                // take the db model as original
                if (modified) { *modified = true; }
                CAircraftModel dbModelModified(dbModel);
                dbModelModified.updateMissingParts(model);
                dbModelModified.setDistributorOrder(distributorOrder);
                dbModelModified.setSimulator(dbModel.getSimulator()); // DB simulator settings have priority
                return dbModelModified;
            }

            // we try our best to update by DB data here
            // since we have no(!) DB model, we update each of it subobjects
            CAircraftModel consolidatedModel(model); // copy over
            if (!consolidatedModel.getLivery().hasValidDbKey())
            {
                const CLivery dbLivery(sApp->getWebDataServices()->smartLiverySelector(consolidatedModel.getLivery()));
                if (dbLivery.hasValidDbKey())
                {
                    if (modified) { *modified = true; }
                    consolidatedModel.setLivery(dbLivery);
                }
            }
            if (!consolidatedModel.getAircraftIcaoCode().hasValidDbKey() && consolidatedModel.hasAircraftDesignator())
            {
                // try to find DB aircraft ICAO here
                const CAircraftIcaoCode dbIcao(sApp->getWebDataServices()->smartAircraftIcaoSelector(consolidatedModel.getAircraftIcaoCode()));
                if (dbIcao.hasValidDbKey())
                {
                    if (modified) { *modified = true; }
                    consolidatedModel.setAircraftIcaoCode(dbIcao);
                }
            }

            const CDistributor dbDistributor(sApp->getWebDataServices()->getDistributors().smartDistributorSelector(model.getDistributor(), model));
            if (dbDistributor.isLoadedFromDb())
            {
                if (modified) { *modified = true; }
                consolidatedModel.setDistributor(dbDistributor);
            }
            consolidatedModel.updateLocalFileNames(model);
            consolidatedModel.setDistributorOrder(distributorOrder);
            return consolidatedModel;
        }

        int CDatabaseUtils::consolidateModelsWithDbData(CAircraftModelList &models, bool force)
        {
            return CDatabaseUtils::consolidateModelsWithDbDataAllowsGuiRefresh(models, force, false);
        }

        CAircraftModelList CDatabaseUtils::consolidateModelsWithSimulatorModelsAllowsGuiRefresh(const CAircraftModelList &models, const CAircraftModelList &simulatorModels, bool processEvents)
        {
            QTime timer;
            timer.start();
            if (models.isEmpty() || simulatorModels.isEmpty()) { return models; }

            const QSet<QString> allOwnModelsModelStrings = simulatorModels.getModelStringSet();
            CAircraftModelList consolidatedModels;

            int c = 0;
            for (const CAircraftModel &model : models)
            {
                c++;
                if (processEvents && c % 125 == 0) { sApp->processEventsFor(25); }

                const QString ms(model.getModelString());
                if (ms.isEmpty()) { continue; }
                if (!allOwnModelsModelStrings.contains(ms)) { continue; }
                consolidatedModels.push_back(model);
            }
            CLogMessage(getLogCategories()).info("Consolidated %1  vs. %2 in %3 ms") << models.size() << simulatorModels.size() << timer.elapsed() << "ms";
            return consolidatedModels;
        }

        int CDatabaseUtils::consolidateModelsWithDbDataAllowsGuiRefresh(CAircraftModelList &models, bool force, bool processEvents)
        {
            QTime timer;
            timer.start();
            int c = 0;
            if (models.isEmpty()) { return c; }
            for (CAircraftModel &model : models)
            {
                if (!force && model.isLoadedFromDb()) { continue; }
                bool modified = false;
                model = CDatabaseUtils::consolidateModelWithDbData(model, force, &modified);
                if (modified || model.hasValidDbKey())
                {
                    c++;
                    if (processEvents && c % 125 == 0) { sApp->processEventsFor(25); }
                }
            }
            CLogMessage(getLogCategories()).info("Consolidated %1 models in %2 ms") << models.size() << timer.elapsed();
            return c;
        }

        int CDatabaseUtils::consolidateModelsWithDbData(const CAircraftModelList &dbModels, CAircraftModelList &simulatorModels, bool force)
        {
            QTime timer;
            timer.start();
            if (dbModels.isEmpty() || simulatorModels.isEmpty()) { return 0; }
            const QSet<QString> dbModelsModelStrings = dbModels.getModelStringSet();

            int c = 0;
            for (CAircraftModel &model : simulatorModels)
            {
                const QString ms(model.getModelString());
                if (ms.isEmpty()) { continue; }
                if (!dbModelsModelStrings.contains(ms)) { continue; }
                bool modified = false;
                const CAircraftModel consolidated = CDatabaseUtils::consolidateModelWithDbData(model, dbModels.findFirstByModelStringOrDefault(ms), force, &modified);
                if (!modified) { continue; }
                model = consolidated;
                c++;
            }
            CLogMessage(getLogCategories()).info("Consolidated %1 models in %2 ms") << simulatorModels.size() << timer.elapsed();
            return c;
        }

        int CDatabaseUtils::consolidateModelsWithDbModelAndDistributor(CAircraftModelList &models, bool force)
        {
            int c = 0;
            if (models.isEmpty()) { return c; }
            for (CAircraftModel &model : models)
            {
                if (!force && model.hasValidDbKey()) { continue; }
                const CAircraftModel dbModel(sApp->getWebDataServices()->getModelForModelString(model.getModelString()));
                if (dbModel.hasValidDbKey())
                {
                    model = dbModel;
                    c++;
                    continue;
                }
                const CDistributor distributor = sApp->getWebDataServices()->smartDistributorSelector(model.getDistributor(), model);
                if (distributor.isLoadedFromDb())
                {
                    model.setDistributor(distributor);
                }
            }
            return c;
        }

        CAircraftModel CDatabaseUtils::consolidateModelWithDbDistributor(const CAircraftModel &model, bool force)
        {
            if (!force && model.getDistributor().isLoadedFromDb()) { return model; }
            const CDistributor distributor = sApp->getWebDataServices()->smartDistributorSelector(model.getDistributor(), model);
            if (!distributor.isLoadedFromDb()) { return model; }
            CAircraftModel newModel(model);
            newModel.setDistributor(distributor);
            return newModel;
        }

        int CDatabaseUtils::consolidateModelsWithDbDistributor(CAircraftModelList &models, bool force)
        {
            int c = 0;
            if (models.isEmpty()) { return c; }
            for (CAircraftModel &model : models)
            {
                if (model.hasValidDbKey() || model.getDistributor().hasValidDbKey()) { continue; }
                model = CDatabaseUtils::consolidateModelWithDbDistributor(model, force);
                if (model.getDistributor().hasValidDbKey()) { c++; }
            }
            return c;
        }

        CAircraftModelList CDatabaseUtils::updateSimulatorForFsFamily(const CAircraftModelList &ownModels, int maxToStash, IProgressIndicator *progressIndicator, bool processEvents)
        {
            CAircraftModelList dbFsFamilyModels(sApp->getWebDataServices()->getModels().getAllFsFamilyModels());
            CAircraftModelList stashModels;
            if (dbFsFamilyModels.isEmpty() || ownModels.isEmpty()) { return stashModels; }
            const QSet<QString> dbKeys = dbFsFamilyModels.getModelStringSet();
            const int mexModelsCount = maxToStash >= 0 ? maxToStash : ownModels.size();
            if (mexModelsCount < 1) { return stashModels; }

            int c = 0; // counter
            for (const CAircraftModel &ownModel : ownModels)
            {
                c++;

                // process events
                if (processEvents && c % 500 == 0)
                {
                    if (progressIndicator)
                    {
                        const int percentage = c * 100 / mexModelsCount;
                        progressIndicator->updateProgressIndicatorAndProcessEvents(percentage);
                    }
                    else
                    {
                        sApp->processEventsFor(10);
                    }
                }

                // values to be skipped
                if (maxToStash >= 0 && maxToStash == stashModels.size()) { break; }
                if (!dbKeys.contains(ownModel.getModelString())) { continue; }
                if (ownModel.matchesSimulatorFlag(CSimulatorInfo::XPLANE)) { continue; }

                // in DB
                CAircraftModel dbModel = dbFsFamilyModels.findFirstByModelStringOrDefault(ownModel.getModelString());
                if (!dbModel.isLoadedFromDb()) {continue; }
                if (dbModel.getSimulator() == ownModel.getSimulator()) {continue; }

                // update simulator and add
                CSimulatorInfo simulator(dbModel.getSimulator());
                simulator.add(ownModel.getSimulator());
                dbModel.setSimulator(simulator);
                stashModels.push_back(dbModel);
            }
            return stashModels;
        }

        QJsonDocument CDatabaseUtils::databaseJsonToQJsonDocument(const QString &content)
        {
            static const QString compressed("swift:");
            if (content.isEmpty()) { return QJsonDocument(); }
            QByteArray byteData;
            if (Json::looksLikeJson(content))
            {
                // uncompressed
                byteData = content.toUtf8();
            }
            else if (content.startsWith(compressed) && content.length() > compressed.length() + 3)
            {
                do
                {
                    // "swift:1234:base64encoded
                    const int cl = compressed.length();
                    const int contentIndex = content.indexOf(':', cl);
                    if (contentIndex < cl) break; // should not happen, malformed
                    const QString ls = content.mid(cl, contentIndex - cl); // content length
                    bool ok;
                    const qint32 size = ls.toInt(&ok);
                    if (!ok) break; // malformed size
                    if (size < 1) break;

                    // Length header, unsigned, big-endian, 32-bit integer
                    QByteArray lengthHeader;
                    QDataStream stream(&lengthHeader, QIODevice::WriteOnly);
                    stream.setByteOrder(QDataStream::BigEndian);
                    stream << size;

                    QByteArray ba;
                    ba.append(content.mid(contentIndex));
                    ba = QByteArray::fromBase64(ba);
                    ba.insert(0, lengthHeader); // adding 4 bytes length header
                    byteData = qUncompress(ba);
                }
                while (false);

            }
            if (byteData.isEmpty()) { return QJsonDocument(); }
            return QJsonDocument::fromJson(byteData);
        }

        QJsonDocument CDatabaseUtils::readQJsonDocumentFromDatabaseFile(const QString &filename)
        {
            const QString raw = CFileUtils::readFileToString(filename);
            if (raw.isEmpty()) { return QJsonDocument(); }
            return CDatabaseUtils::databaseJsonToQJsonDocument(raw);
        }

        QJsonObject CDatabaseUtils::readQJsonObjectFromDatabaseFile(const QString &filename)
        {
            const QString raw = CFileUtils::readFileToString(filename);
            if (raw.isEmpty()) { return QJsonObject(); }
            return BlackMisc::Json::jsonObjectFromString(raw);
        }

        QJsonObject CDatabaseUtils::readQJsonObjectFromDatabaseFile(const QString &directory, const QString &filename)
        {
            return CDatabaseUtils::readQJsonObjectFromDatabaseFile(CFileUtils::appendFilePaths(directory, filename));
        }

        bool CDatabaseUtils::hasDbAircraftData()
        {
            return sApp && sApp->hasWebDataServices() && sApp->getWebDataServices()->hasDbAircraftData();
        }

        const QUrlQuery &CDatabaseUtils::getCompressedQuery()
        {
            static const QUrlQuery q("compressed=true");
            return q;
        }

        QHttpPart CDatabaseUtils::getJsonTextMultipart(const QJsonObject &json, bool compress)
        {
            const QByteArray bytes(QJsonDocument(json).toJson(QJsonDocument::Compact));
            return getJsonTextMultipart(bytes, compress);
        }

        QHttpPart CDatabaseUtils::getJsonTextMultipart(const QJsonArray &json, bool compress)
        {
            const QByteArray bytes(QJsonDocument(json).toJson(QJsonDocument::Compact));
            return getJsonTextMultipart(bytes, compress);
        }

        QHttpPart CDatabaseUtils::getJsonTextMultipart(const QByteArray &bytes, bool compress)
        {
            static const QString name("form-data; name=\"swiftjson\"");
            static const QVariant header(name);
            QHttpPart textPart;
            textPart.setHeader(QNetworkRequest::ContentDispositionHeader, header);
            if (compress)
            {
                QByteArray ba = qCompress(bytes);
                ba.remove(0, 4); // remove the non standard header
                textPart.setBody(ba);
            }
            else
            {
                textPart.setBody(bytes);
            }
            return textPart;
        }

        QHttpPart CDatabaseUtils::getMultipartWithDebugFlag()
        {
            QHttpPart textPartDebug;
            textPartDebug.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"XDEBUG_SESSION_START\""));
            textPartDebug.setBody(QString("ECLIPSE_DBGP").toUtf8());
            return textPartDebug;
        }
    } // ns
} // ns
