#include <enet/enet.h>
#include <iostream>
#include <string>
#include <map>

enum class MsgHeader { GameStart, Redirect, PlayerJoinedGame, AllPlayers, PlayersPings, SendPositionUpdate, PingCheck, PingAnswer, SuccessConnection, UpdatePositions };

void BroadcastString(ENetHost* server, std::string& str)
{
    char* data = str.data();
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, packet);
}

void BroadcastFragment(ENetHost* server, std::string& str)
{
    char* data = str.data();
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_host_broadcast(server, 1, packet);
}

void SendString(ENetPeer* peer, std::string& str)
{
    char* data = str.data();
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

void SendFragment(ENetPeer* peer, std::string& str)
{
    char* data = str.data();
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(peer, 0, packet);
}

int main(int argc, const char** argv)
{
    if (enet_initialize() != 0)
    {
        printf("Cannot init ENet");
        return 1;
    }
    atexit(enet_deinitialize);
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 10888;
    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
    if (!server)
    {
        printf("Cannot create ENet server\n");
        return 1;
    }

    uint32_t timeStart = enet_time_get();
    uint32_t lastPingSended = timeStart;
    uint32_t lastPingInfoSended = timeStart;
    uint32_t lastPositionInfoSended = timeStart;

    int playerIds[100];
    int players = 0;
    std::map<int, std::string> names{};
    std::map<int, int> pings{};
    std::map<int, int> last_check{};
    std::map<int, std::pair<int, int>> positions{};

    while (true)
    {
        ENetEvent event;
        uint32_t curTime = enet_time_get();
        while (enet_host_service(server, &event, 10) > 0)
        {
            std::string data;
            std::string delimiter = "||";
            std::string token;
            MsgHeader header;
            float position;
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                playerIds[players] = players;
                event.peer->data = playerIds + players;
                names[players] = std::string("Player") + std::to_string(players);
                pings[players] = 0;
                last_check[players] = curTime;
                players += 1;
                std::cout << "Connected player " << *(int*)event.peer->data << " named " << names[*(int*)event.peer->data] << std::endl;

                data = std::to_string((int)MsgHeader::PlayerJoinedGame) + "||" + names[*(int*)event.peer->data] + ":" + std::to_string(*(int*)event.peer->data);
                BroadcastString(server, data);
                data = std::to_string((int)MsgHeader::SuccessConnection) + "||" + names[*(int*)event.peer->data] + ":" + std::to_string(*(int*)event.peer->data);
                SendString(event.peer, data);

                data = std::to_string((int)MsgHeader::AllPlayers) + "||";
                for (auto i : names) {
                    data += i.second + ":" + std::to_string(i.first) + ",";
                }
                SendString(event.peer, data);

                break;
            case ENET_EVENT_TYPE_RECEIVE:
                data = (char*)event.packet->data;

                token = data.substr(0, data.find(delimiter));
                header = (MsgHeader)std::stoi(token);

                switch (header)
                {
                case MsgHeader::PingAnswer:
                    data.erase(0, data.find(delimiter) + delimiter.length());
                    last_check[*(int*)event.peer->data] = curTime;
                    pings[*(int*)event.peer->data] = curTime - std::stoi(data);
                    break;
                case MsgHeader::SendPositionUpdate:
                    data.erase(0, data.find(delimiter) + delimiter.length());
                    delimiter = ":";
                    position = std::stof(data.substr(0, data.find(delimiter)));
                    data.erase(0, data.find(delimiter) + delimiter.length());
                    positions[*(int*)event.peer->data] = std::pair<int, int>{ (int)position, (int)std::stof(data) };
                    break;
                default:
                    break;
                }
                enet_packet_destroy(event.packet);
                break;
            default:
                break;
            };
        }

        if (curTime - lastPingInfoSended > 1000) {
            lastPingInfoSended = curTime;
            std::string request = std::to_string((int)MsgHeader::PlayersPings) + "||";
            for (auto i : names) {
                if (curTime - last_check[i.first] > 1000) {
                    request += i.second + ":" + std::to_string(1000) + ",";
                }
                else {
                    request += i.second + ":" + std::to_string(pings[i.first]) + ",";
                }

            }
            BroadcastFragment(server, request);
        }


        if (curTime - lastPositionInfoSended > 1000) {
            lastPositionInfoSended = curTime;
            std::string request = std::to_string((int)MsgHeader::UpdatePositions) + "||";
            for (auto i : names) {
                request += i.second + ":" + std::to_string(positions[i.first].first) + ":" + std::to_string(positions[i.first].second) + ",";
            }
            BroadcastFragment(server, request);
        }


        if (curTime - lastPingSended > 1000) {
            lastPingSended = curTime;
            std::string request = std::to_string((int)MsgHeader::PingCheck) + "||" + std::to_string(curTime);
            BroadcastFragment(server, request);
        }
    }

    enet_host_destroy(server);
    atexit(enet_deinitialize);

    return 0;
}