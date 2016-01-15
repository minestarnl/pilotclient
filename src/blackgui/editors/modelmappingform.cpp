/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "modelmappingform.h"
#include "ui_modelmappingform.h"

using namespace BlackMisc;
using namespace BlackMisc::Simulation;

namespace BlackGui
{
    namespace Editors
    {
        CModelMappingForm::CModelMappingForm(QWidget *parent) :
            CForm(parent),
            ui(new Ui::CModelMappingForm)
        {
            ui->setupUi(this);
            ui->le_LastUpdated->setReadOnly(true);
            ui->le_Id->setReadOnly(true);
            ui->lai_Id->set(CIcons::appMappings16(), "Id:");
            connect(ui->pb_Stash, &QPushButton::clicked, this, &CModelMappingForm::requestStash);
        }

        CModelMappingForm::~CModelMappingForm()
        { }

        BlackMisc::Simulation::CAircraftModel CModelMappingForm::getValue() const
        {
            CAircraftModel model(m_originalModel);
            model.setSimulatorInfo(this->ui->selector_Simulator->getValue());
            model.setDescription(this->ui->le_Description->text());
            model.setModelString(this->ui->le_ModelKey->text());
            model.setName(this->ui->le_Name->text());
            return model;
        }

        CStatusMessageList CModelMappingForm::validate(bool withNestedForms) const
        {
            CAircraftModel model(getValue());
            CStatusMessageList msgs(model.validate(withNestedForms));
            this->ui->val_Indicator->setState(msgs);
            return msgs;
        }

        void CModelMappingForm::setReadOnly(bool readOnly)
        {
            this->ui->le_Description->setReadOnly(readOnly);
            this->ui->le_ModelKey->setReadOnly(readOnly);
            this->ui->le_Name->setReadOnly(readOnly);
        }

        void CModelMappingForm::setSelectOnly()
        {
            this->setReadOnly(true);
        }

        void CModelMappingForm::setValue(BlackMisc::Simulation::CAircraftModel &model)
        {
            ui->le_ModelKey->setText(model.getModelString());
            ui->le_LastUpdated->setText(model.getFormattedUtcTimestampYmdhms());
            ui->le_Id->setText(model.getDbKeyAsString());
            ui->le_Description->setText(model.getDescription());
            ui->le_Name->setText(model.getName());
            ui->selector_Simulator->setValue(model.getSimulatorInfo());
            m_originalModel = model;
        }
    } // ns
} // ns
