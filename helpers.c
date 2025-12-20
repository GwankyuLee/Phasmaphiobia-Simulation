#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include "helpers.h"

// ---- House layout ----
void house_populate_rooms(struct House* house) {
    // Willow House layout from Phasmaphobia, DO NOT MODIFY HOUSE LAYOUT
    house->room_count = 13;

    room_init(house->rooms+0, "Van", true);
    room_init(house->rooms+1, "Hallway", false);
    room_init(house->rooms+2, "Master Bedroom", false);
    room_init(house->rooms+3, "Boy's Bedroom", false);
    room_init(house->rooms+4, "Bathroom", false);
    room_init(house->rooms+5, "Basement", false);
    room_init(house->rooms+6, "Basement Hallway", false);
    room_init(house->rooms+7, "Right Storage Room", false);
    room_init(house->rooms+8, "Left Storage Room", false);
    room_init(house->rooms+9, "Kitchen", false);
    room_init(house->rooms+10, "Living Room", false);
    room_init(house->rooms+11, "Garage", false);
    room_init(house->rooms+12, "Utility Room", false);

    room_connect(house->rooms+0, house->rooms+1);    // Van - Hallway
    room_connect(house->rooms+1, house->rooms+2);    // Hallway - Master Bedroom
    room_connect(house->rooms+1, house->rooms+3);    // Hallway - Boy's Bedroom
    room_connect(house->rooms+1, house->rooms+4);    // Hallway - Bathroom
    room_connect(house->rooms+1, house->rooms+9);    // Hallway - Kitchen
    room_connect(house->rooms+1, house->rooms+5);    // Hallway - Basement
    room_connect(house->rooms+5, house->rooms+6);    // Basement - Basement Hallway
    room_connect(house->rooms+6, house->rooms+7);    // Basement Hallway - Right Storage Room
    room_connect(house->rooms+6, house->rooms+8);    // Basement Hallway - Left Storage Room
    room_connect(house->rooms+9, house->rooms+10);   // Kitchen - Living Room
    room_connect(house->rooms+9, house->rooms+11);   // Kitchen - Garage
    room_connect(house->rooms+11, house->rooms+12);  // Garage - Utility Room

    house->starting_room = house->rooms; // Van is at index 0
}

// ---- to_string functions ----
const char* evidence_to_string(enum EvidenceType evidence) {
    switch (evidence) {
        case EV_EMF:
            return "emf";
        case EV_ORBS:
            return "orbs";
        case EV_RADIO:
            return "radio";
        case EV_TEMPERATURE:
            return "temp";
        case EV_FINGERPRINTS:
            return "prints";
        case EV_WRITING:
            return "writing";
        case EV_INFRARED:
            return "infrared";
        default:
            return "unknown";
    }
}

const char* ghost_to_string(enum GhostType ghost) {
    switch (ghost) {
        case GH_POLTERGEIST:
            return "poltergeist";
        case GH_THE_MIMIC:
            return "the_mimic";
        case GH_HANTU:
            return "hantu";
        case GH_JINN:
            return "jinn";
        case GH_PHANTOM:
            return "phantom";
        case GH_BANSHEE:
            return "banshee";
        case GH_GORYO:
            return "goryo";
        case GH_BULLIES:
            return "bullies";
        case GH_MYLING:
            return "myling";
        case GH_OBAKE:
            return "obake";
        case GH_YUREI:
            return "yurei";
        case GH_ONI:
            return "oni";
        case GH_MOROI:
            return "moroi";
        case GH_REVENANT:
            return "revenant";
        case GH_SHADE:
            return "shade";
        case GH_ONRYO:
            return "onryo";
        case GH_THE_TWINS:
            return "the_twins";
        case GH_DEOGEN:
            return "deogen";
        case GH_THAYE:
            return "thaye";
        case GH_YOKAI:
            return "yokai";
        case GH_WRAITH:
            return "wraith";
        case GH_RAIJU:
            return "raiju";
        case GH_MARE:
            return "mare";
        case GH_SPIRIT:
            return "spirit";
        default:
            return "unknown";
    }
}

const char* exit_reason_to_string(enum LogReason reason) {
    switch (reason) {
        case LR_EVIDENCE:
            return "evidence";
        case LR_BORED:
            return "bored";
        case LR_AFRAID:
            return "afraid";
        default:
            return "unknown";
    }
}

// ---- enum retrieval functions ----
int get_all_evidence_types(const enum EvidenceType** list) {
    // Stored in the data segment so that we can point to it safely
    static const enum EvidenceType evidence_types[] = {
        EV_EMF,
        EV_ORBS,
        EV_RADIO,
        EV_TEMPERATURE,
        EV_FINGERPRINTS,
        EV_WRITING,
        EV_INFRARED
    };

    if (list) {
        *list = evidence_types;
    }
    return (int)(sizeof(evidence_types) / sizeof(evidence_types[0]));
}

int get_all_ghost_types(const enum GhostType** list) {
    // Stored in the data segment so that we can point to it safely
    static const enum GhostType ghost_types[] = {
        GH_POLTERGEIST,
        GH_THE_MIMIC,
        GH_HANTU,
        GH_JINN,
        GH_PHANTOM,
        GH_BANSHEE,
        GH_GORYO,
        GH_BULLIES,
        GH_MYLING,
        GH_OBAKE,
        GH_YUREI,
        GH_ONI,
        GH_MOROI,
        GH_REVENANT,
        GH_SHADE,
        GH_ONRYO,
        GH_THE_TWINS,
        GH_DEOGEN,
        GH_THAYE,
        GH_YOKAI,
        GH_WRAITH,
        GH_RAIJU,
        GH_MARE,
        GH_SPIRIT
    };

    if (list) {
        *list = ghost_types;
    }
    return (int)(sizeof(ghost_types) / sizeof(ghost_types[0]));
}

// ---- Thread-safe random number generation ----
int rand_int_threadsafe(int lower_inclusive, int upper_exclusive) {
    static _Thread_local unsigned seed = 0;

    if (upper_exclusive <= lower_inclusive) {
        return lower_inclusive;
    }

    if (seed == 0) {
        seed = (unsigned)time(NULL) ^ (unsigned)(uintptr_t)pthread_self();
        if (seed == 0) {
            seed = 0xA5A5A5A5u;
        }
    }

    unsigned span = (unsigned)(upper_exclusive - lower_inclusive);
    unsigned value = (unsigned)rand_r(&seed) % span;
    return lower_inclusive + (int)value;
}

// ---- Evidence helpers ----
bool evidence_is_valid_ghost(EvidenceByte mask) {
    const enum GhostType* ghost_types = NULL;
    int ghost_count = get_all_ghost_types(&ghost_types);

    for (int index = 0; index < ghost_count; index++) {
        if (mask == (EvidenceByte)ghost_types[index]) {
            return true;
        }
    }

    return false;
}

// ---- Logging (Writes CSV logs, DO NOT MODIFY the file outputs: timestamp,type,id,room,device,boredom,fear,action,extra) ----

// These enums are just for logging purposes, not needed elsewhere
enum LogEntityType {
    LOG_ENTITY_HUNTER = 0,
    LOG_ENTITY_GHOST = 1
};

struct LogRecord {
    enum LogEntityType entity_type;
    int                entity_id;
    const char*        room;
    const char*        device;
    int                boredom;
    int                fear;
    const char*        action;
    const char*        extra;
};

static const char* log_entity_type_to_string(enum LogEntityType type) {
    switch (type) {
        case LOG_ENTITY_HUNTER:
            return "hunter";
        case LOG_ENTITY_GHOST:
            return "ghost";
        default:
            return "unknown";
    }
}

static void write_log_record(const struct LogRecord* record) {
    static _Thread_local unsigned line_count = 0;

    if (line_count >= 100000) {
        fprintf(stderr, "Log capped for entity %d; stopping to prevent infinite growth.\n", record->entity_id);
        exit(1);
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "log_%d.csv", record->entity_id);

    FILE* log_file = fopen(filename, "a");

    if (!log_file) {
        return;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long timestamp = (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;

    const char* entity = log_entity_type_to_string(record->entity_type);
    const char* room = record->room ? record->room : "";
    const char* device = record->device ? record->device : "";
    const char* action = record->action ? record->action : "";
    const char* extra = record->extra ? record->extra : "";

    fprintf(log_file,
            "%lld,%s,%d,%s,%s,%d,%d,%s,%s\n",
            timestamp,
            entity,
            record->entity_id,
            room,
            device,
            record->boredom,
            record->fear,
            action,
            extra);

    fclose(log_file);
    line_count++;

    // Short pause helps ensure successive logs receive distinct timestamps.
    struct timespec pause = {0, 2 * 1000 * 1000}; // 2 ms
    nanosleep(&pause, NULL);
}

void log_move(int hunter_id, int boredom, int fear, const char* from_room, const char* to_room, enum EvidenceType device) {
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = hunter_id,
        .room = from_room,
        .device = evidence_to_string(device),
        .boredom = boredom,
        .fear = fear,
        .action = "MOVE",
        .extra = to_room
    };

    write_log_record(&record);

    printf("Hunter %d using %s moved from %s to %s (bored=%d fear=%d)\n",
           hunter_id,
           evidence_to_string(device),
           from_room ? from_room : "",
           to_room ? to_room : "",
           boredom,
           fear);
}

void log_evidence(int hunter_id, int boredom, int fear, const char* room_name, enum EvidenceType device) {
    const char* evidence = evidence_to_string(device);
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = hunter_id,
        .room = room_name,
        .device = evidence,
        .boredom = boredom,
        .fear = fear,
        .action = "EVIDENCE",
        .extra = evidence
    };

    write_log_record(&record);

    printf("Hunter %d using %s gathered evidence in %s (bored=%d fear=%d)\n",
           hunter_id,
           evidence,
           room_name ? room_name : "",
           boredom,
           fear);
}

void log_swap(int hunter_id, int boredom, int fear, enum EvidenceType from_device, enum EvidenceType to_device) {
    char extra[64];
    const char* from_text = evidence_to_string(from_device);
    const char* to_text = evidence_to_string(to_device);
    snprintf(extra, sizeof(extra), "%s->%s", from_text, to_text);

    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = hunter_id,
        .room = NULL,
        .device = to_text,
        .boredom = boredom,
        .fear = fear,
        .action = "SWAP",
        .extra = extra
    };

    write_log_record(&record);

    printf("Hunter %d swapped devices: %s -> %s (bored=%d fear=%d)\n",
           hunter_id,
           from_text,
           to_text,
           boredom,
           fear);
}

void log_exit(int hunter_id, int boredom, int fear, const char* room_name, enum EvidenceType device, enum LogReason reason) {
    const char* device_text = evidence_to_string(device);
    const char* reason_text = exit_reason_to_string(reason);

    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = hunter_id,
        .room = room_name,
        .device = device_text,
        .boredom = boredom,
        .fear = fear,
        .action = "EXIT",
        .extra = reason_text
    };

    write_log_record(&record);

    printf("Hunter %d using %s exited at %s (reason=%s, bored=%d fear=%d)\n",
           hunter_id,
           device_text,
           room_name ? room_name : "",
           reason_text,
           boredom,
           fear);
}

void log_return_to_van(int hunter_id, int boredom, int fear, const char* room_name, enum EvidenceType device, bool heading_home) {
    const char* device_text = evidence_to_string(device);
    const char* extra = heading_home ? "start" : "complete";
    const char* action = heading_home ? "RETURN_START" : "RETURN_COMPLETE";

    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = hunter_id,
        .room = room_name,
        .device = device_text,
        .boredom = boredom,
        .fear = fear,
        .action = action,
        .extra = extra
    };

    write_log_record(&record);

    if (heading_home) {
        printf("Hunter %d using %s heading to van from %s (bored=%d fear=%d)\n",
               hunter_id,
               device_text,
               room_name ? room_name : "",
               boredom,
               fear);
    } else {
        printf("Hunter %d using %s finished return at %s (bored=%d fear=%d)\n",
               hunter_id,
               device_text,
               room_name ? room_name : "",
               boredom,
               fear);
    }
}

void log_hunter_init(int hunter_id, const char* room_name, const char* hunter_name, enum EvidenceType device) {
    const char* device_text = evidence_to_string(device);
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = hunter_id,
        .room = room_name,
        .device = device_text,
        .boredom = 0,
        .fear = 0,
        .action = "INIT",
        .extra = hunter_name ? hunter_name : ""
    };

    write_log_record(&record);
    printf("Hunter %d (%s) initialized in %s with %s\n",
           hunter_id,
           hunter_name ? hunter_name : "unknown",
           room_name ? room_name : "",
           device_text);
}

void log_ghost_init(int ghost_id, const char* room_name, enum GhostType type) {
    const char* type_text = ghost_to_string(type);
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_GHOST,
        .entity_id = ghost_id,
        .room = room_name,
        .device = NULL,
        .boredom = 0,
        .fear = 0,
        .action = "INIT",
        .extra = type_text
    };

    write_log_record(&record);
    printf("Ghost %d (%s) initialized in %s\n",
           ghost_id,
           type_text,
           room_name ? room_name : "");
}

void log_ghost_move(int ghost_id, int boredom, const char* from_room, const char* to_room) {
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_GHOST,
        .entity_id = ghost_id,
        .room = from_room,
        .device = NULL,
        .boredom = boredom,
        .fear = 0,
        .action = "MOVE",
        .extra = to_room
    };

    write_log_record(&record);

    printf("Ghost %d [bored=%d] MOVE %s -> %s\n",
           ghost_id,
           boredom,
           from_room ? from_room : "",
           to_room ? to_room : "");
}

void log_ghost_evidence(int ghost_id, int boredom, const char* room_name, enum EvidenceType evidence) {
    const char* evidence_text = evidence_to_string(evidence);

    struct LogRecord record = {
        .entity_type = LOG_ENTITY_GHOST,
        .entity_id = ghost_id,
        .room = room_name,
        .device = NULL,
        .boredom = boredom,
        .fear = 0,
        .action = "EVIDENCE",
        .extra = evidence_text
    };

    write_log_record(&record);

    printf("Ghost %d [bored=%d] EVIDENCE %s in %s\n",
           ghost_id,
           boredom,
           evidence_text,
           room_name ? room_name : "");
}

void log_ghost_exit(int ghost_id, int boredom, const char* room_name) {
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_GHOST,
        .entity_id = ghost_id,
        .room = room_name,
        .device = NULL,
        .boredom = boredom,
        .fear = 0,
        .action = "EXIT",
        .extra = ""
    };

    write_log_record(&record);

    printf("Ghost %d [bored=%d] EXIT %s\n",
           ghost_id,
           boredom,
           room_name ? room_name : "");
}

void log_ghost_idle(int ghost_id, int boredom, const char* room_name) {
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_GHOST,
        .entity_id = ghost_id,
        .room = room_name,
        .device = NULL,
        .boredom = boredom,
        .fear = 0,
        .action = "IDLE",
        .extra = ""
    };

    write_log_record(&record);

    printf("Ghost %d [bored=%d] IDLE in %s\n",
           ghost_id,
           boredom,
           room_name ? room_name : "");
}
