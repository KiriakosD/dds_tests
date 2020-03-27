/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include <ace/Log_Msg.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsSubscriptionC.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "DataReaderListenerImpl.h"
#include "MessengerTypeSupportImpl.h"
#include <iostream>
#include <thread>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void RunSub(DDS::DomainParticipant_var participant, DDS::Topic_var topic, int id);
int RunPub(DDS::DomainParticipant_var participant, DDS::Topic_var topic, int id);

int main()
{

    std::cout << "starting up " << std::endl;
    int domain_id = 42;
    int num_args = 11;

    srand(time(NULL));

    int randval = rand() % 100;
    std::cout << "id:" << randval << std::endl;

    ACE_TCHAR *args[num_args] = {
        "./subscriber", "-ORBDebugLevel", "0", "-DCPSDebugLevel",
        "0", "-DCPSTransportDebugLevel", "0", "-ORBLogFile",
        "subscriber.log", "-DCPSConfigFile", "rtps.ini"};

    try
    {
        // Initialize DomainParticipantFactory
        DDS::DomainParticipantFactory_var dpf =
            TheParticipantFactoryWithArgs(num_args, args);

        // Create DomainParticipant
        DDS::DomainParticipant_var participant =
            dpf->create_participant(domain_id,
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

        // Register Type (Messenger::Message)
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
        std::thread sub(RunPub, participant, topic, randval);
        std::thread pub(RunSub, participant, topic, randval);

        while (true)
        {
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

void RunSub(DDS::DomainParticipant_var participant, DDS::Topic_var topic, int id)
{
    std::cout << "sub starting" << std::endl;
    // Create Subscriber
    DDS::Subscriber_var subscriber =
        participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                       0,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!subscriber)
    {
        std::cout << "shits whaked yo!!!";
        return;
    }

    // Create DataReader
    DDS::DataReaderListener_var listener(new DataReaderListenerImpl);

    DDS::DataReaderQos reader_qos;
    subscriber->get_default_datareader_qos(reader_qos);
    reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::ContentFilteredTopic_var cft =
        participant->create_contentfilteredtopic("MyTopic-Filtered",
                                                 topic,
                                                 "subject_id <> 4",
                                                 DDS::StringSeq());
    DDS::DataReader_var reader =
        subscriber->create_datareader(cft,
                                      reader_qos,
                                      listener,
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!reader)
    {
        std::cout << "shits whaked yo!!!";
        return;
    }

    Messenger::MessageDataReader_var reader_i =
        Messenger::MessageDataReader::_narrow(reader);

    if (!reader_i)
    {
        std::cout << "shits whaked yo!!!";
        return;
    }

    // Block until Publisher completes
    DDS::StatusCondition_var condition = reader->get_statuscondition();
    condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    while (true)
    {
        DDS::SubscriptionMatchedStatus matches;
        if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK)
        {
            std::cout << "shits whaked yo!!!";
            return;
        }
        if (matches.current_count_change != 0)
        {
            std::cout << "current publishers: " << matches.current_count << "\n";
        }

        if (matches.current_count == 0 && matches.total_count > 0)
        {
            // Wait until Publisher sends stuff
            //DDS::StatusCondition_var condition = reader->get_statuscondition();
            //condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

            //DDS::WaitSet_var ws = new DDS::WaitSet;
            ws->attach_condition(condition);
        }

        DDS::ConditionSeq conditions;
        DDS::Duration_t timeout = {60, 0};
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
        {
            std::cout << "shits whaked yo!!!";
            return;
        }
    }

    // after everything is finnished
    ws->detach_condition(condition);
    std::cout << "sub finnished" << std::endl;
}

int RunPub(DDS::DomainParticipant_var participant, DDS::Topic_var topic, int id)
{
    std::cout << "pub starting" << std::endl;
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

    // Create DataWriter
    DDS::DataWriter_var writer =
        publisher->create_datawriter(topic,
                                     DATAWRITER_QOS_DEFAULT,
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
    // message.subject_id = rand() % 100;
    message.subject_id = id;

    message.from = "Comic Book Guy";
    message.subject = "Review";
    message.text = "Worst. Movie. Ever.";
    message.count = 0;
    std::cout << "entering phase 2 \n";

    for (int i = 0; i < 10; i++)
    {
        DDS::PublicationMatchedStatus matches;
        writer->get_publication_matched_status(matches);
        if (matches.current_count_change != 0)
        {
            std::cout << "matches currently: " << matches.current_count << "\n";
        }
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
        usleep(100);
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
    std::cout << "pub finnished" << std::endl;
}