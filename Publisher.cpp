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

    // Create Topic (Movie Discussion List)
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name,
                                  TOPIC_QOS_DEFAULT,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                            ACE_TEXT(" create_topic failed!\n")),
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
