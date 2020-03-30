/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL_H
#define DATAREADER_LISTENER_IMPL_H

#include <ace/Global_Macros.h>

#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DdsDcpsSubscriptionC.h>

// Mine
#include <vector>
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

class DataReaderListenerImpl
    : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  virtual void on_requested_deadline_missed(
      DDS::DataReader_ptr reader,
      const DDS::RequestedDeadlineMissedStatus &status);

  virtual void on_requested_incompatible_qos(
      DDS::DataReader_ptr reader,
      const DDS::RequestedIncompatibleQosStatus &status);

  virtual void on_sample_rejected(DDS::DataReader_ptr reader,
                                  const DDS::SampleRejectedStatus &status);

  virtual void on_liveliness_changed(
      DDS::DataReader_ptr reader, const DDS::LivelinessChangedStatus &status);

  virtual void on_data_available(DDS::DataReader_ptr reader);

  virtual void on_subscription_matched(
      DDS::DataReader_ptr reader, const DDS::SubscriptionMatchedStatus &status);

  virtual void on_sample_lost(DDS::DataReader_ptr reader,
                              const DDS::SampleLostStatus &status);

  Messenger::Message getLastMessage();

private:
  Messenger::Message m_LastMessage;
};

#endif /* DATAREADER_LISTENER_IMPL_H */
