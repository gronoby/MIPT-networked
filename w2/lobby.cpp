#include <enet/enet.h>
#include <iostream>
#include <string>

enum class MsgHeader { GameStart, Redirect, PlayerJoinedGame, AllPlayers, PlayersPings, SendPositionUpdate, PingCheck, PingAnswer, SuccessConnection, UpdatePositions };

void BroadcastString(ENetHost* server, std::string& str)
{
    char* data = str.data();
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, packet);
}

void SendString(ENetPeer* peer, std::string& str)
{
    char* data = str.data();
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
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
    address.port = 10887;
    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
    if (!server)
    {
        printf("Cannot create ENet server\n");
        return 1;
    }

    bool game_started = false;

    while (true)
    {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0)
        {
            std::string data;
            std::string delimiter = "||";
            std::string token;
            MsgHeader header;
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                if (game_started) {
                    printf("Sending game server data\n");
                    data = std::to_string((int)MsgHeader::Redirect) + "||localhost:10888";
                    SendString(event.peer, data);
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                data = (char*)event.packet->data;

                token = data.substr(0, data.find(delimiter));
                header = (MsgHeader)std::stoi(token);

                switch (header)
                {
                case MsgHeader::GameStart:
                    if (!game_started) {
                        game_started = true;
                        printf("Game start request: starting the game\n");
                        printf("Sending game server data to all current piers\n");
                        data = std::to_string((int)MsgHeader::Redirect) + "||localhost:10888";
                        BroadcastString(server, data);
                    }
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
    }

    enet_host_destroy(server);
    atexit(enet_deinitialize);

    return 0;
}