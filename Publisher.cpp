/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <iostream>
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "MessengerTypeSupportImpl.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  //std::cout << "Pub arguments: " << argc << "\n";
  ACE_TCHAR *args[9] = {
      "./publisher", "-ORBDebugLevel", "10", "-DCPSDebugLevel", "10", "-ORBLogFile",
      "publisher.log", "-DCPSConfigFile", "rtps.ini"};
  argc = 9;
  try
  {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, args);

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
        dpf->create_participant(42,
                                PARTICIPANT_QOS_DEFAULT,
                                0,
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" create_participant failed!\n")),
                       1);
    }

    // Register TypeSupport (Messenger::Message)
    Messenger::MessageTypeSupport_var ts =
        new Messenger::MessageTypeSupportImpl;

    if (ts->register_type(participant, "") != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" register_type failed!\n")),
                       1);
    }

    //Testing QOS on topic
    DDS::TopicQos tpc_qos;
    participant->get_default_topic_qos(tpc_qos);
    tpc_qos.transport_priority.value = 10;

    // Create Topic (Movie Discussion List)
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name,
                                  tpc_qos,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" create_topic failed!\n")),
                       1);
    }

    // Create Publisher
    DDS::Publisher_var publisher =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      0,
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!publisher)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" create_publisher failed!\n")),
                       1);
    }

    //Testing QOS on DataWritter
    DDS::DataWriterQos dw_qos;
    publisher->get_default_datawriter_qos(dw_qos);
    dw_qos.transport_priority.value = 10;
    dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    dw_qos.reliability.max_blocking_time.sec = 10;
    dw_qos.reliability.max_blocking_time.nanosec = 0;
    dw_qos.resource_limits.max_samples_per_instance = 100;

    // Create DataWriter
    DDS::DataWriter_var writer =
        publisher->create_datawriter(topic,
                                     dw_qos,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" create_datawriter failed!\n")),
                       1);
    }

    Messenger::MessageDataWriter_var message_writer =
        Messenger::MessageDataWriter::_narrow(writer);

    if (!message_writer)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" _narrow failed!\n")),
                       1);
    }

    // Block until Subscriber is available
    DDS::StatusCondition_var condition = writer->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    while (true)
    {
      DDS::PublicationMatchedStatus matches;
      if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK)
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" get_publication_matched_status failed!\n")),
                         1);
      }

      if (matches.current_count >= 1)
      {
        break;
      }

      DDS::ConditionSeq conditions;
      DDS::Duration_t timeout = {60, 0};
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                              ACE_TEXT(" wait failed!\n")),
                         1);
      }
    }

    ws->detach_condition(condition);

    /* initialize random seed: */
    srand(time(NULL));

    // Write samples
    Messenger::Message message;
    message.subject_id = rand() % 100;

    message.from = "Comic Book Guy";
    message.subject = "Review";
    message.text = "Worst. Movie. Ever.";
    message.count = 0;

    for (int i = 0; i < 1000; ++i)
    {
      DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);
      ++message.count;
      //++message.subject_id;

      if (error != DDS::RETCODE_OK)
      {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("ERROR: %N:%l: main() -")
                       ACE_TEXT(" write returned %d!\n"),
                   error));
      }
    }

    // Wait for samples to be acknowledged
    DDS::Duration_t timeout = {30, 0};
    if (message_writer->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" wait_for_acknowledgments failed!\n")),
                       1);
    }

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception &e)
  {
    e._tao_print_exception("Exception caught in main():");
    return 1;
  }

  return 0;
}
