//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2012 Filipe Coelho (falktx@openoctave.org)
//=========================================================

#ifndef __PLUGIN_LV2_H__
#define __PLUGIN_LV2_H__

#include "plugin.h"
#include "plugingui.h"
#include "song.h"

#include "lv2_data_access.h"
#include "lv2_event.h"
#include "lv2_event_helpers.h"
#include "lv2_external_ui.h"
#include "lv2_instance_access.h"
#include "lv2_state.h"
#include "lv2_time.h"
#include "lv2_ui_resize.h"
#include "lv2_uri_map.h"
#include "lv2_urid.h"

#include <QWidget>

#ifdef GTK2UI_SUPPORT
#undef signals
#include <gtk/gtk.h>
static void oom_lv2_gtk_window_destroyed(void*, void* data)
{
    if (data) ((Lv2Plugin*)data)->closeNativeGui(true);
}
#endif

#define LV2_NS_ATOM  "http://lv2plug.in/ns/ext/atom#"
#define LV2_NS_UI    "http://lv2plug.in/ns/extensions/ui#"

// define extra URIs not yet in lilv
#define LILV_URI_TIME_EVENT   "http://lv2plug.in/ns/ext/time#TimeEvent"

// static max values
const unsigned int MAX_EVENT_BUFFER = 0x7FFF; // 32767

// feature ids
const uint16_t lv2_feature_id_uri_map         = 0;
const uint16_t lv2_feature_id_urid_map        = 1;
const uint16_t lv2_feature_id_urid_unmap      = 2;
const uint16_t lv2_feature_id_event           = 3;
const uint16_t lv2_feature_id_data_access     = 4;
const uint16_t lv2_feature_id_instance_access = 5;
const uint16_t lv2_feature_id_ui_resize       = 6;
const uint16_t lv2_feature_id_ui_parent       = 7;
const uint16_t lv2_feature_id_external_ui     = 8;
const uint16_t lv2_feature_id_external_ui_old = 9;
const uint16_t lv2_feature_count              = 10;

// uri[d] map ids
const uint16_t OOM_URI_MAP_ID_EVENT_MIDI      = 1; // 0x1
const uint16_t OOM_URI_MAP_ID_EVENT_TIME      = 2; // 0x2
const uint16_t OOM_URI_MAP_ID_ATOM_STRING     = 3;
const uint16_t OOM_URI_MAP_ID_COUNT           = 4;

// extra plugin hints
const unsigned int PLUGIN_HAS_EXTENSION_STATE = 0x100;

//static LV2_Time_Position oom_lv2_time_pos = { 0, 0, LV2_TIME_STOPPED, 0, 0, 0, 0, 0, 0, 0.0 };

struct LV2World {
    LilvWorld* world;

    LilvNode* connectionOptional;
    LilvNode* inPlaceBroken;
    LilvNode* integer;
    LilvNode* reportsLatency;
    LilvNode* sampleRate;
    LilvNode* toggled;

    LilvNode* portInput;
    LilvNode* portOutput;
    LilvNode* portAudio;
    LilvNode* portControl;
    LilvNode* portEvent;
    LilvNode* portEventMidi;
    LilvNode* portEventTime;

    LilvNode* uiGtk2;
    LilvNode* uiQt4;
    LilvNode* uiX11;
    LilvNode* uiExternal;
    LilvNode* uiExternalOld;

//    LilvNode* logarithmic_prop;
//    LilvNode* units_prop;
//    LilvNode* persist_prop;

    //LilvInstance* instance;
    const LilvPlugins* plugins;
};

static LV2World* lv2world = 0;

void initLV2()
{
    lv2world = new LV2World;

    lv2world->world = lilv_world_new();
    lilv_world_load_all(lv2world->world);

    lv2world->connectionOptional = lilv_new_uri(lv2world->world, LILV_NS_LV2 "connectionOptional");
    lv2world->inPlaceBroken  = lilv_new_uri(lv2world->world, LILV_NS_LV2 "inPlaceBroken");
    lv2world->integer        = lilv_new_uri(lv2world->world, LILV_NS_LV2 "integer");
    lv2world->reportsLatency = lilv_new_uri(lv2world->world, LILV_NS_LV2 "reportsLatency");
    lv2world->sampleRate = lilv_new_uri(lv2world->world, LILV_NS_LV2 "sampleRate");
    lv2world->toggled    = lilv_new_uri(lv2world->world, LILV_NS_LV2 "toggled");

    lv2world->portInput     = lilv_new_uri(lv2world->world, LILV_URI_INPUT_PORT);
    lv2world->portOutput    = lilv_new_uri(lv2world->world, LILV_URI_OUTPUT_PORT);
    lv2world->portAudio     = lilv_new_uri(lv2world->world, LILV_URI_AUDIO_PORT);
    lv2world->portControl   = lilv_new_uri(lv2world->world, LILV_URI_CONTROL_PORT);
    lv2world->portEvent     = lilv_new_uri(lv2world->world, LILV_URI_EVENT_PORT);
    lv2world->portEventMidi = lilv_new_uri(lv2world->world, LILV_URI_MIDI_EVENT);
    lv2world->portEventTime = lilv_new_uri(lv2world->world, LILV_URI_TIME_EVENT);

    lv2world->uiGtk2 = lilv_new_uri(lv2world->world, LV2_NS_UI "GtkUI");
    lv2world->uiQt4  = lilv_new_uri(lv2world->world, LV2_NS_UI "Qt4UI");
    lv2world->uiX11  = lilv_new_uri(lv2world->world, LV2_NS_UI "X11UI");
    lv2world->uiExternal    = lilv_new_uri(lv2world->world, LV2_EXTERNAL_UI_URI);
    lv2world->uiExternalOld = lilv_new_uri(lv2world->world, LV2_EXTERNAL_UI_DEPRECATED_URI);

//    lv2world->logarithmic_prop = lilv_new_uri(lv2world->world, "http://lv2plug.in/ns/dev/extportinfo#logarithmic");
//    lv2world->persist_prop = lilv_new_uri(lv2world->world,  LV2_PERSIST_URI);

    lv2world->plugins = lilv_world_get_all_plugins(lv2world->world);

    LILV_FOREACH(plugins, i, lv2world->plugins)
    {
        const LilvPlugin* p = lilv_plugins_get(lv2world->plugins, i);
        LilvNode* name = lilv_plugin_get_name(p);

        QString p_uri = QString(lilv_node_as_string(lilv_plugin_get_uri(p)));
        QString p_name = QString(name ? lilv_node_as_string(name) : "");

        if (name)
            lilv_node_free(name);

        // Make sure it doesn't already exist.
        if (plugins.find(p_uri, p_name) == 0)
        {
            plugins.add(PLUGIN_LV2, p_uri, p_name, p);
        }
    }
}

bool isLV2FeatureSupported(const char* uri)
{
    if (strcmp(uri, "http://lv2plug.in/ns/lv2core#hardRTCapable") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/lv2core#inPlaceBroken") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/lv2core#isLive") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/event") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/state#makePath") == 0)
        return false; // TODO
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/state#mapPath") == 0)
        return false; // TODO
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/uri-map") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/urid#map") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/urid#unmap") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/data-access") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/instance-access") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/ui-resize") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/extensions/ui#Events") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/extensions/ui#makeResident") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/extensions/ui#makeSONameResident") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/extensions/ui#noUserResize") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/extensions/ui#fixedSize") == 0)
        return true;
    else if (strcmp(uri, "http://lv2plug.in/ns/extensions/ui#external") == 0)
        return true;
    else if (strcmp(uri, "http://nedko.arnaudov.name/lv2/external_ui/") == 0)
        return true;
    else
        return false;
}

// ----------------- LV2 Fifo -------------------------------------------------------
#if 0
#define LV2_FIFO_SIZE 1024

struct LV2Data
{
    uint32_t frame;
    unsigned time;
    float value;
};

class LV2ControlFifo
{
    LV2ControlFifo()
    {
        clear();
    }

    ~LV2ControlFifo()
    {
    }

    bool isEmpty() const
    {
        return size == 0;
    }

    void clear()
    {
        size = index = rindex = 0;
    }

    int getSize() const
    {
        return size;
    }

    bool put(const LV2Data& event)
    {
        if (size < LV2_FIFO_SIZE)
        {
            fifo[index] = event;
            index = (index + 1) % LV2_FIFO_SIZE;
            ++size;
            return true;
        }
        return false;
    }

    LV2Data get()
    {
        LV2Data event(fifo[rindex]);
        rindex = (rindex + 1) % LV2_FIFO_SIZE;
        --size;
        return event;
    }

    const LV2Data& peek(uint32_t n)
    {
        uint32_t idx = (rindex + n) % LV2_FIFO_SIZE;
        return fifo[idx];

    }

    void remove()
    {
        rindex = (rindex + 1) % LV2_FIFO_SIZE;
        --size;
    }

private:
    LV2Data fifo[LV2_FIFO_SIZE];
    volatile int size;
    uint32_t index;
    uint32_t rindex;
};
#endif

// ----------------- URI-Map Feature -------------------------------------------------
static uint32_t oom_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
{
    qDebug("oom_lv2_uri_to_id(%p, %s, %s)", data, map, uri);

    if (map && strcmp(map, LV2_EVENT_URI) == 0)
    {
        // Event types
        if (strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent") == 0)
            return OOM_URI_MAP_ID_EVENT_MIDI;
        else if (strcmp(uri, "http://lv2plug.in/ns/ext/time#Position") == 0)
            return OOM_URI_MAP_ID_EVENT_TIME;
    }
    else if (strcmp(uri, LV2_NS_ATOM "String") == 0)
    {
        return OOM_URI_MAP_ID_ATOM_STRING;
    }

    // Custom types
    if (data)
    {
        Lv2Plugin* plugin = (Lv2Plugin*)data;
        return plugin->getCustomURIId(uri);
    }

    return 0;
}

// ----------------- URID Feature ----------------------------------------------------
static LV2_URID oom_lv2_urid_map(LV2_URID_Map_Handle handle, const char* uri)
{
    qDebug("oom_lv2_urid_map(%p, %s)", handle, uri);

    if (strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent") == 0)
        return OOM_URI_MAP_ID_EVENT_MIDI;
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/time#Position") == 0)
        return OOM_URI_MAP_ID_EVENT_TIME;
    else if (strcmp(uri, LV2_NS_ATOM "String") == 0)
        return OOM_URI_MAP_ID_ATOM_STRING;

    // Custom types
    if (handle)
    {
        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        return plugin->getCustomURIId(uri);
    }

    return 0;
}

static const char* oom_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
{
    qDebug("oom_lv2_urid_unmap(%p, %i)", handle, urid);

    if (urid == OOM_URI_MAP_ID_EVENT_MIDI)
        return "http://lv2plug.in/ns/ext/midi#MidiEvent";
    else if (urid == OOM_URI_MAP_ID_EVENT_TIME)
        return "http://lv2plug.in/ns/ext/time#Position";
    else if (urid == OOM_URI_MAP_ID_ATOM_STRING)
        return LV2_NS_ATOM "String";

    // Custom types
    if (handle)
    {
        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        return plugin->getCustomURIString(urid);
    }

    return 0;
}

// ----------------- Event Feature ---------------------------------------------------
static uint32_t oom_lv2_event_ref(LV2_Event_Callback_Data data, LV2_Event* event)
{
    qDebug("oom_lv2_event_ref(%p, %p)", data, event);

    // TODO
    return 0;
}

static uint32_t oom_lv2_event_unref(LV2_Event_Callback_Data data, LV2_Event* event)
{
    qDebug("oom_lv2_event_unref(%p, %p)", data, event);

    // TODO
    return 0;
}

// ----------------- State Feature ---------------------------------------------------
static int oom_lv2_state_store(LV2_State_Handle handle, uint32_t key, const void* value, size_t size, uint32_t type, uint32_t flags)
{
    qDebug("oom_lv2_state_store(%p, %i, %p, %lu, %i, %i)", handle, key, value, size, type, flags);

    if (handle)
    {
        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        const char* uri_key = plugin->getCustomURIString(key);

        if (uri_key > 0 && (flags & LV2_STATE_IS_POD) > 0)
        {
            qDebug("oom_lv2_state_store(%p, %i, %p, %lu, %i, %i) - Got uri_key and flags", handle, key, value, size, type, flags);

            int dtype;

            if (type == OOM_URI_MAP_ID_ATOM_STRING)
                dtype = 0;  // string
            else if (type >= OOM_URI_MAP_ID_COUNT)
                dtype = 1;  // binary
            else
                dtype = -1; // invalid

            if (value && dtype != -1)
            {
                if (dtype == 0)
                {
                    // string
                    plugin->saveState(uri_key, (const char*)value);
                }
                else
                {   // binary
                    QByteArray chunk((const char*)value, size);
                    plugin->saveState(uri_key, chunk.toBase64().data());
                }

                return 0;
            }
            else
                qCritical("oom_lv2_state_store(%p, %i, %p, %lu, %i, %i) - Invalid data", handle, key, value, size, type, flags);
        }
        else
            qWarning("oom_lv2_state_store(%p, %i, %p, %lu, %i, %i) - Invalid attributes", handle, key, value, size, type, flags);
    }
    else
        qCritical("oom_lv2_state_store(%p, %i, %p, %lu, %i, %i) - Invalid handle", handle, key, value, size, type, flags);

    return 1;
}

static const void* oom_lv2_state_retrieve(LV2_State_Handle handle, uint32_t key, size_t* size, uint32_t* type, uint32_t* flags)
{
    qDebug("oom_lv2_state_retrieve(%p, %i, %p, %p, %p)", handle, key, size, type, flags);
}

// ----------------- External UI Feature ---------------------------------------------
static void oom_lv2_external_ui_closed(LV2UI_Controller controller)
{
    qDebug("oom_lv2_external_ui_closed(%p)", controller);

    if (controller)
    {
        Lv2Plugin* plugin = (Lv2Plugin*)controller;
        plugin->closeNativeGui();
    }
}

// ----------------- UI Resize Feature -----------------------------------------------
static int oom_lv2_ui_resize(LV2_UI_Resize_Feature_Data data, int width, int height)
{
    qDebug("oom_lv2_ui_resized(%p, %i, %i)", data, width, height);

    if (data)
    {
        Lv2Plugin* plugin = (Lv2Plugin*)data;
        plugin->ui_resize(width, height);
        return 0;
    }

    return 1;
}

// ----------------- UI Extension ----------------------------------------------------
static void oom_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
{
    qDebug("oom_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);

    if (controller)
    {
        Lv2Plugin* plugin = (Lv2Plugin*)controller;
        plugin->ui_write_function(port_index, buffer_size, format, buffer);
    }
}

Lv2Plugin::Lv2Plugin()
{
    m_type = PLUGIN_LV2;
    m_paramsBuffer = 0;

    m_customURIs.clear();

    // Fill pre-set URI keys
    for (uint16_t i=0; i < OOM_URI_MAP_ID_COUNT; i++)
        m_customURIs.append(0);

    handle = 0;
    descriptor = 0;

    ui.type = UI_NONE;
    ui.visible = false;
    ui.nativeWidget = 0;
    ui.handle = 0;
    ui.widget = 0;
    ui.descriptor = 0;
    ui._lib = 0;

    for (uint16_t i=0; i < lv2_feature_count+1; i++)
        features[i] = 0;

    lplug = 0;
}

Lv2Plugin::~Lv2Plugin()
{
    // close UI
    if (m_hints & PLUGIN_HAS_NATIVE_GUI)
    {
        if (ui.handle && ui.descriptor && ui.descriptor->cleanup)
            ui.descriptor->cleanup(ui.handle);

        switch (ui.type)
        {
#ifdef GTK2UI_SUPPORT
        case UI_GTK2:
            if (ui.nativeWidget)
                gtk_widget_destroy((GtkWidget*)ui.nativeWidget);
            break;
#endif
        case UI_X11:
            delete (QWidget*)ui.nativeWidget;
            break;
        case UI_EXTERNAL:
            //if (ui.handle && ui.widget)
            //    LV2_EXTERNAL_UI_HIDE((lv2_external_ui*)ui.widget);
            break;
        default:
            break;
        }

        if (features[lv2_feature_id_data_access] && features[lv2_feature_id_data_access]->data)
            delete (LV2_Extension_Data_Feature*)features[lv2_feature_id_data_access]->data;

        if (features[lv2_feature_id_ui_resize] && features[lv2_feature_id_ui_resize]->data)
            delete (LV2_UI_Resize_Feature*)features[lv2_feature_id_ui_resize]->data;

        if (features[lv2_feature_id_external_ui] && features[lv2_feature_id_external_ui]->data)
            delete (lv2_external_ui_host*)features[lv2_feature_id_external_ui]->data;

        if (ui._lib)
            lib_close(ui._lib);
    }

    for (int i=0; i < m_customURIs.count(); i++)
    {
        if (m_customURIs[i])
            free((void*)m_customURIs[i]);
    }

    m_customURIs.clear();

    if (handle && descriptor->deactivate && m_activeBefore)
        descriptor->deactivate(handle);

    if (handle && descriptor->cleanup)
        descriptor->cleanup(handle);

    if (features[lv2_feature_id_uri_map] && features[lv2_feature_id_uri_map]->data)
        delete (LV2_URI_Map_Feature*)features[lv2_feature_id_uri_map]->data;

    if (features[lv2_feature_id_urid_map] && features[lv2_feature_id_urid_map]->data)
        delete (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;

    if (features[lv2_feature_id_urid_unmap] && features[lv2_feature_id_urid_unmap]->data)
        delete (LV2_URID_Unmap*)features[lv2_feature_id_urid_unmap]->data;

    if (features[lv2_feature_id_event] && features[lv2_feature_id_event]->data)
        delete (LV2_Event_Feature*)features[lv2_feature_id_event]->data;

    for (uint16_t i=0; i<lv2_feature_count && features[i]; i++)
        delete features[i];

    if (m_paramCount > 0)
        delete[] m_paramsBuffer;

    m_audioInIndexes.clear();
    m_audioOutIndexes.clear();
}

void Lv2Plugin::initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle)
{
    LilvPlugin* lv2plug = (LilvPlugin*)nativeHandle;

    uint32_t portCount = lilv_plugin_get_num_ports(lv2plug);
    for (uint32_t i = 0; i < portCount; i++)
    {
        const LilvPort* port = lilv_plugin_get_port_by_index(lv2plug, i);

        if (port)
        {
            if (lilv_port_is_a(lv2plug, port, lv2world->portAudio))
            {
                if(lilv_port_is_a(lv2plug, port, lv2world->portInput))
                    plugi->m_audioInputCount += 1;
                else if(lilv_port_is_a(lv2plug, port, lv2world->portOutput))
                    plugi->m_audioOutputCount += 1;
            }
        }
    }

    plugi->m_name = QString(label);

    LilvNode* lv2maker = lilv_plugin_get_author_name(lv2plug);
    if (lv2maker)
    {
        plugi->m_maker = QString(lilv_node_as_string(lv2maker));
        lilv_node_free(lv2maker);
    }

    const LilvPluginClass* lilvClass = lilv_plugin_get_class(lv2plug);
    const LilvNode* lilvClassLabel = lilv_plugin_class_get_label(lilvClass);
    const char* lv2Class = lilv_node_as_string(lilvClassLabel);

    if (strcmp(lv2Class, "Instrument") == 0)
        plugi->m_hints |= PLUGIN_IS_SYNTH;
    else if (plugi->m_audioInputCount >= 1 && plugi->m_audioOutputCount >= 1)
        plugi->m_hints |= PLUGIN_IS_FX;

    if (lilv_plugin_has_feature(lv2plug, lv2world->inPlaceBroken))
        plugi->m_hints |= PLUGIN_HAS_IN_PLACE_BROKEN;

    if (lilv_plugin_get_uis(lv2plug))
        plugi->m_hints |= PLUGIN_HAS_NATIVE_GUI;

    Q_UNUSED(filename);
}

bool Lv2Plugin::init(QString filename, QString label)
{
    if (!lplug)
    {
        LilvNode* pluginURI = lilv_new_uri(lv2world->world, filename.toAscii().constData());
        lplug = lilv_plugins_get_by_uri(lv2world->plugins, pluginURI);
        lilv_node_free(pluginURI);
    }

    if (lplug)
    {
        _lib = lib_open(lilv_uri_to_path(lilv_node_as_uri(lilv_plugin_get_library_uri(lplug))));

        if (_lib)
        {
            LV2_Descriptor_Function descfn = (LV2_Descriptor_Function) lib_symbol(_lib, "lv2_descriptor");

            if (descfn)
            {
                uint32_t i = 0;
                const char* c_uri = strdup(filename.toAscii().constData());

                while ((descriptor = descfn(i++)))
                {
                    if (strcmp(descriptor->URI, c_uri) == 0)
                        break;
                }

                if (descriptor)
                {
                    bool can_continue = true;

                    // Check supported ports
                    uint32_t PortCount = lilv_plugin_get_num_ports(lplug);
                    for (i=0; i < PortCount; i++)
                    {
                        const LilvPort* port = lilv_plugin_get_port_by_index(lplug, i);

                        if (port)
                        {
                            if (lilv_port_is_a(lplug, port, lv2world->portAudio) == false && lilv_port_is_a(lplug, port, lv2world->portControl) == false && lilv_port_is_a(lplug, port, lv2world->portEvent) == false)
                            {
                                if (lilv_port_has_property(lplug, port, lv2world->connectionOptional) == false)
                                {
                                    set_last_error("Plugin requires a port that is not currently supported");
                                    can_continue = false;
                                    break;
                                }
                            }
                        }
                    }

                    // Check supported features
                    if (can_continue)
                    {
                        LilvNodes* reqFeatures = lilv_plugin_get_required_features(lplug);
                        if (reqFeatures)
                        {
                            LILV_FOREACH(nodes, j, reqFeatures)
                            {
                                const char* f_uri = lilv_node_as_uri(lilv_nodes_get(reqFeatures, j));
                                if (isLV2FeatureSupported(f_uri) == false)
                                {
                                    QString msg = QString("Plugin requires a feature that is not supported:\n%1").arg(f_uri);
                                    set_last_error(msg.toAscii().constData());
                                    can_continue = false;
                                    break;
                                }

                            }
                            lilv_nodes_free(reqFeatures);
                        }
                    }

                    if (can_continue)
                    {
                        // Initialize features
                        LV2_URI_Map_Feature* URI_Map_Feature = new LV2_URI_Map_Feature;
                        URI_Map_Feature->callback_data       = this;
                        URI_Map_Feature->uri_to_id           = oom_lv2_uri_to_id;

                        LV2_URID_Map* URID_Map_Feature       = new LV2_URID_Map;
                        URID_Map_Feature->handle             = this;
                        URID_Map_Feature->map                = oom_lv2_urid_map;

                        LV2_URID_Unmap* URID_Unmap_Feature   = new LV2_URID_Unmap;
                        URID_Unmap_Feature->handle           = this;
                        URID_Unmap_Feature->unmap            = oom_lv2_urid_unmap;

                        LV2_Event_Feature* Event_Feature     = new LV2_Event_Feature;
                        Event_Feature->callback_data         = this;
                        Event_Feature->lv2_event_ref         = oom_lv2_event_ref;
                        Event_Feature->lv2_event_unref       = oom_lv2_event_unref;

                        features[lv2_feature_id_uri_map]          = new LV2_Feature;
                        features[lv2_feature_id_uri_map]->URI     = LV2_URI_MAP_URI;
                        features[lv2_feature_id_uri_map]->data    = URI_Map_Feature;

                        features[lv2_feature_id_urid_map]         = new LV2_Feature;
                        features[lv2_feature_id_urid_map]->URI    = LV2_URID_MAP_URI;
                        features[lv2_feature_id_urid_map]->data   = URID_Map_Feature;

                        features[lv2_feature_id_urid_unmap]       = new LV2_Feature;
                        features[lv2_feature_id_urid_unmap]->URI  = LV2_URID_UNMAP_URI;
                        features[lv2_feature_id_urid_unmap]->data = URID_Unmap_Feature;

                        features[lv2_feature_id_event]            = new LV2_Feature;
                        features[lv2_feature_id_event]->URI       = LV2_EVENT_URI;
                        features[lv2_feature_id_event]->data      = Event_Feature;

                        handle = descriptor->instantiate(descriptor, sampleRate, lilv_node_as_string(lilv_plugin_get_bundle_uri(lplug)), features);

                        if (handle)
                        {
                            // store information
                            m_name  = label;
                            m_label = label;
                            m_filename = filename;

                            LilvNode* lv2maker = lilv_plugin_get_author_name(lplug);
                            if (lv2maker)
                            {
                                m_maker = QString(lilv_node_as_string(lv2maker));
                                lilv_node_free(lv2maker);
                            }

                            // reload port information
                            reload();

                            // try to find an usable UI
                            const LilvUI *uiQt4, *uiX11, *uiExt, *uiExtOld, *uiGtk2, *uiFinal;
                            uiQt4 = uiX11 = uiExt = uiExtOld = uiGtk2 = uiFinal = 0;

                            LilvUIs* UIs = lilv_plugin_get_uis(lplug);
                            LILV_FOREACH(uis, u, UIs)
                            {
                                const LilvUI* this_ui = lilv_uis_get(UIs, u);

                                if (lilv_ui_is_a(this_ui, lv2world->uiGtk2))
                                    uiGtk2 = this_ui;
                                else if (lilv_ui_is_a(this_ui, lv2world->uiQt4))
                                    uiQt4 = this_ui;
                                else if (lilv_ui_is_a(this_ui, lv2world->uiX11))
                                    uiX11 = this_ui;
                                else if (lilv_ui_is_a(this_ui, lv2world->uiExternal))
                                    uiExt = this_ui;
                                else if (lilv_ui_is_a(this_ui, lv2world->uiExternalOld))
                                    uiExtOld = this_ui;
                            }

                            if (uiQt4)
                            {
                                uiFinal = uiQt4;
                                ui.type = UI_QT4;
                            }
                            else if (uiX11)
                            {
                                uiFinal = uiX11;
                                ui.type = UI_X11;
                            }
                            else if (uiExt)
                            {
                                uiFinal = uiExt;
                                ui.type = UI_EXTERNAL;
                            }
                            else if (uiExtOld)
                            {
                                uiFinal = uiExtOld;
                                ui.type = UI_EXTERNAL;
                            }
                            else if (uiGtk2)
                            {
                                uiFinal = uiGtk2;
                                ui.type = UI_GTK2;
                            }

                            // Use proper UI now
                            if (uiFinal)
                            {
                                ui._lib = lib_open(lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_binary_uri(uiFinal))));

                                if (ui._lib)
                                {
                                    LV2UI_DescriptorFunction ui_descfn = (LV2UI_DescriptorFunction) lib_symbol(ui._lib, "lv2ui_descriptor");

                                    if (ui_descfn)
                                    {
                                        const char* c_ui_uri = strdup(lilv_node_as_uri(lilv_ui_get_uri(uiFinal)));
                                        ui.bundlePath = QString(lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_bundle_uri(uiFinal))));

                                        i = 0;
                                        while ((ui.descriptor = ui_descfn(i++)))
                                        {
                                            if (strcmp(ui.descriptor->URI, c_ui_uri) == 0)
                                                break;
                                        }

                                        free((void*)c_ui_uri);

                                        if (ui.descriptor)
                                        {
                                            // Create base widget if needed
                                            switch (ui.type)
                                            {
                                            case UI_X11:
                                                ui.nativeWidget = new QWidget();
                                                break;
                                            default:
                                                break;
                                            }

                                            // Initialize UI features
                                            LV2_Extension_Data_Feature* UI_Data_Feature = new LV2_Extension_Data_Feature;
                                            UI_Data_Feature->data_access                = descriptor->extension_data;

                                            LV2_UI_Resize_Feature* UI_Resize_Feature    = new LV2_UI_Resize_Feature;
                                            UI_Resize_Feature->data                     = this;
                                            UI_Resize_Feature->ui_resize                = oom_lv2_ui_resize;

                                            lv2_external_ui_host* External_UI_Feature   = new lv2_external_ui_host;
                                            External_UI_Feature->ui_closed              = oom_lv2_external_ui_closed;
                                            External_UI_Feature->plugin_human_id        = strdup(m_name.toAscii().constData());

                                            features[lv2_feature_id_data_access]           = new LV2_Feature;
                                            features[lv2_feature_id_data_access]->URI      = LV2_DATA_ACCESS_URI;
                                            features[lv2_feature_id_data_access]->data     = UI_Data_Feature;

                                            features[lv2_feature_id_instance_access]       = new LV2_Feature;
                                            features[lv2_feature_id_instance_access]->URI  = LV2_INSTANCE_ACCESS_URI;
                                            features[lv2_feature_id_instance_access]->data = handle;

                                            features[lv2_feature_id_ui_resize]             = new LV2_Feature;
                                            features[lv2_feature_id_ui_resize]->URI        = LV2_UI_RESIZE_URI "#UIResize";
                                            features[lv2_feature_id_ui_resize]->data       = UI_Resize_Feature;

                                            features[lv2_feature_id_ui_parent]             = new LV2_Feature;
                                            features[lv2_feature_id_ui_parent]->URI        = LV2_UI_URI "#parent";
                                            features[lv2_feature_id_ui_parent]->data       = (ui.type == UI_X11) ? (void*)((QWidget*)ui.nativeWidget)->winId() : 0;

                                            features[lv2_feature_id_external_ui]           = new LV2_Feature;
                                            features[lv2_feature_id_external_ui]->URI      = LV2_EXTERNAL_UI_URI;
                                            features[lv2_feature_id_external_ui]->data     = External_UI_Feature;

                                            features[lv2_feature_id_external_ui_old]       = new LV2_Feature;
                                            features[lv2_feature_id_external_ui_old]->URI  = LV2_EXTERNAL_UI_DEPRECATED_URI;
                                            features[lv2_feature_id_external_ui_old]->data = External_UI_Feature;

                                            m_hints |= PLUGIN_HAS_NATIVE_GUI;

                                            // wait for showNativeGui() to init UI
                                        }
                                        else
                                            lib_close(ui._lib);
                                    }
                                    else
                                        lib_close(ui._lib);
                                }
                            } // iFinal

                            // plugin is valid
                            return true;
                        }
                        else
                        {
                            descriptor = 0;
                            set_last_error("Plugin failed to initialize");
                        }
                    }
                    else
                    {
                        descriptor = 0;
                        // last error already set, missing features
                    }
                }
                else
                    set_last_error("Failed to find the requested plugin URI in the plugin library");
            }
            else
                set_last_error("Not a LV2 plugin");
        }
        else
            set_last_error(lib_error());
    }
    else
        set_last_error("Failed to find plugin in the LV2 collection");

    return false;
}

void Lv2Plugin::reload()
{
    // delete old data
    if (m_paramCount > 0)
    {
        delete[] m_params;
        delete[] m_paramsBuffer;
    }

    m_audioInIndexes.clear();
    m_audioOutIndexes.clear();

    // reset
    m_hints  = 0;
    m_params = 0;
    m_paramsBuffer = 0;
    m_paramCount   = 0;

    // query new data
    uint32_t ains, aouts, params, j;
    ains = aouts = params = 0;

    uint32_t portCount = lilv_plugin_get_num_ports(lplug);
    for (uint32_t i = 0; i < portCount; i++)
    {
        const LilvPort* port = lilv_plugin_get_port_by_index(lplug, i);

        if (port)
        {
            if (lilv_port_is_a(lplug, port, lv2world->portAudio))
            {
                if(lilv_port_is_a(lplug, port, lv2world->portInput))
                    ains += 1;
                else if(lilv_port_is_a(lplug, port, lv2world->portOutput))
                    aouts += 1;
            }
            else if (lilv_port_is_a(lplug, port, lv2world->portControl))
                params += 1;
        }
    }

    // plugin hints
    const LilvPluginClass* lilvClass = lilv_plugin_get_class(lplug);
    const LilvNode* lilvClassLabel = lilv_plugin_class_get_label(lilvClass);
    const char* lv2Class = lilv_node_as_string(lilvClassLabel);

    if (strcmp(lv2Class, "Instrument") == 0)
        m_hints |= PLUGIN_IS_SYNTH;
    else if (ains > 1 && aouts > 1)
        m_hints |= PLUGIN_IS_FX;

    if (lilv_plugin_has_feature(lplug, lv2world->inPlaceBroken))
        m_hints |= PLUGIN_HAS_IN_PLACE_BROKEN;

    // allocate data
    if (params > 0)
    {
        m_params = new ParameterPort[params];
        m_paramsBuffer = new float[params];
    }

    // fill in all the data
    for (uint32_t i = 0; i < portCount; i++)
    {
        const LilvPort* port = lilv_plugin_get_port_by_index(lplug, i);

        if (port)
        {
            if (lilv_port_is_a(lplug, port, lv2world->portAudio))
            {
                if(lilv_port_is_a(lplug, port, lv2world->portInput))
                {
                    m_audioInIndexes.push_back(i);
                }
                else if(lilv_port_is_a(lplug, port, lv2world->portOutput))
                {
                    m_audioOutIndexes.push_back(i);
                }
                else
                    qWarning("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (lilv_port_is_a(lplug, port, lv2world->portControl))
            {
                j = m_paramCount++;
                m_params[j].rindex = i;

                double min, max, def, step, step_small, step_large;

                LilvNode* lmin;
                LilvNode* lmax;
                LilvNode* ldef;
                lilv_port_get_range(lplug, port, &ldef, &lmin, &lmax);

                if (lmin)
                {
                    min = lilv_node_as_float(lmin);
                    lilv_node_free(lmin);
                }
                else
                    min = 0.0;

                if (lmax)
                {
                    max = lilv_node_as_float(lmax);
                    lilv_node_free(lmax);
                }
                else
                    max = 1.0;

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                if (ldef)
                {
                    def = lilv_node_as_float(ldef);
                    lilv_node_free(ldef);
                }
                else
                {
                    if (min < 0.0 && max > 0.0)
                        def = 0.0;
                    else
                        def = min;
                }

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (max - min == 0.0)
                {
                    qWarning("Broken plugin parameter -> max - min == 0");
                    max = min + 0.1;
                }

                // hints
                if (lilv_port_has_property(lplug, port, lv2world->sampleRate))
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    def *= sampleRate;
                    m_params[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                //if (lilv_port_has_property(lplug, port, lv2world->logarithmic))
                //{
                //    m_params[j].hints |= PARAMETER_IS_LOGARITHMIC;
                //}

                if (lilv_port_has_property(lplug, port, lv2world->integer))
                {
                    step = 1.0;
                    step_small = 1.0;
                    step_large = 10.0;
                    m_params[j].hints |= PARAMETER_IS_INTEGER;
                }
                else if (lilv_port_has_property(lplug, port, lv2world->toggled))
                {
                    step = max - min;
                    step_small = step;
                    step_large = step;
                    m_params[j].hints |= PARAMETER_IS_TOGGLED;
                }
                else
                {
                    double range = max - min;
                    step = range/100.0;
                    step_small = range/1000.0;
                    step_large = range/10.0;
                }

                if (lilv_port_is_a(lplug, port, lv2world->portInput))
                {
                    m_params[j].type = PARAMETER_INPUT;
                    m_params[j].hints |= PARAMETER_IS_ENABLED;
                    //m_params[j].hints |= PARAMETER_IS_AUTOMABLE;
                }
                else if (lilv_port_is_a(lplug, port, lv2world->portOutput))
                {
                    m_params[j].type = PARAMETER_OUTPUT;
                    m_params[j].hints |= PARAMETER_IS_ENABLED;

                    if (lilv_port_has_property(lplug, port, lv2world->reportsLatency) == false)
                    {
                        m_params[j].hints |= PARAMETER_IS_AUTOMABLE;
                    }
                    else
                    {
                        // latency parameter
                        min = 0;
                        max = sampleRate;
                        def = 0;
                        step = 1;
                        step_small = 1;
                        step_large = 1;
                    }
                }
                else
                {
                    qWarning("WARNING - Got a broken Port (Control, but not input or output)");
                }

                m_params[j].ranges.min = min;
                m_params[j].ranges.max = max;
                m_params[j].ranges.def = def;
                m_params[j].ranges.step = step;
                m_params[j].ranges.step_small = step_small;
                m_params[j].ranges.step_large = step_large;

                // Start parameters in their default values
                m_params[j].value = m_paramsBuffer[j] = def;

                descriptor->connect_port(handle, i, &m_paramsBuffer[j]);
            }
        }
    }

    reloadPrograms(true);
}

void Lv2Plugin::reloadPrograms(bool /*init*/)
{
    // TODO
}

QString Lv2Plugin::getParameterName(uint32_t index)
{
    if (lplug && index < m_paramCount)
    {
        const LilvPort* port = lilv_plugin_get_port_by_index(lplug, m_params[index].rindex);
        return QString(lilv_node_as_string(lilv_port_get_name(lplug, port)));
    }
    else
        return QString("");
}

void Lv2Plugin::setNativeParameterValue(uint32_t index, double value)
{
    if (descriptor && index < m_paramCount)
        m_paramsBuffer[index] = value;
}

uint32_t Lv2Plugin::getCustomURIId(const char *uri)
{
    qDebug("Lv2Plugin::getCustomURIId(%s)", uri);

    for (int i=0; i < m_customURIs.count(); i++)
    {
        if (m_customURIs[i] && strcmp(m_customURIs[i], uri) == 0)
            return i;
    }

    m_customURIs.append(strdup(uri));
    return m_customURIs.count()-1;
}

const char* Lv2Plugin::getCustomURIString(int uri_id)
{
    qDebug("Lv2Plugin::getCustomURIString(%i)", uri_id);

    if (uri_id < m_customURIs.count())
        return m_customURIs.at(uri_id);
    else
        return 0;
}

void Lv2Plugin::saveState(const char* uri_key, const char* value)
{

}

bool Lv2Plugin::hasNativeGui()
{
    return (m_hints & PLUGIN_HAS_NATIVE_GUI);
}

void Lv2Plugin::showNativeGui(bool yesno)
{
    // Initialize UI if needed
    if (! ui.handle)
    {
        ui.handle = ui.descriptor->instantiate(ui.descriptor, m_filename.toUtf8().constData(), ui.bundlePath.toUtf8().constData(), oom_lv2_ui_write_function, this, &ui.widget, features);

        if (ui.handle)
        {
            if (ui.type == UI_GTK2)
            {
#ifdef GTK2UI_SUPPORT
                GtkWidget* hostWidget   = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                GtkWidget* pluginWidget = (GtkWidget*)ui.widget;

                //gtk_window_set_resizable(GTK_WINDOW(hostWidget), 1);
                gtk_window_set_title(GTK_WINDOW(hostWidget), m_name.toUtf8().constData());
                gtk_container_add(GTK_CONTAINER(hostWidget), pluginWidget);
                g_signal_connect(G_OBJECT(hostWidget), "destroy", G_CALLBACK(oom_lv2_gtk_window_destroyed), this);

                ui.nativeWidget = hostWidget;
#endif
            }
            else if (ui.type == UI_QT4)
            {
                QWidget* pluginWidget = (QWidget*)ui.widget;
                pluginWidget->adjustSize();
                pluginWidget->setAttribute(Qt::WA_DeleteOnClose);
                pluginWidget->setWindowTitle(m_name);
            }
            else if (ui.type == UI_X11)
            {
                QWidget* hostWidget = (QWidget*)ui.nativeWidget;
                hostWidget->setWindowTitle(m_name);
            }
        }
        else
        {
            // UI failed to initialize
            return;
        }
    }

    if (yesno)
        updateUIPorts();

    // Now show it
    switch (ui.type)
    {
#ifdef GTK2UI_SUPPORT
    case UI_GTK2:
        if (yesno)
            gtk_widget_show_all((GtkWidget*)ui.nativeWidget);
        else
            closeNativeGui();
        break;
#endif
    case UI_QT4:
        if (yesno)
            ((QWidget*)ui.widget)->show();
        else
            ((QWidget*)ui.widget)->close();
        break;
    case UI_X11:
        ((QWidget*)ui.nativeWidget)->setVisible(yesno);
        break;
    case UI_EXTERNAL:
        if (yesno)
            LV2_EXTERNAL_UI_SHOW((lv2_external_ui*)ui.widget);
        else
            closeNativeGui();
        break;
    default:
        break;
    }

    ui.visible = yesno;
}

bool Lv2Plugin::nativeGuiVisible()
{
    if (ui.handle && ui.widget)
    {
        switch (ui.type)
        {
#ifdef GTK2UI_SUPPORT
        case UI_GTK2:
            return ui.visible;
#endif
        case UI_QT4:
            return ((QWidget*)ui.widget)->isVisible();
        case UI_X11:
            if (ui.nativeWidget)
                return ((QWidget*)ui.nativeWidget)->isVisible();
            return false;
        case UI_EXTERNAL:
            return ui.visible;
        default:
            break;
        }
    }
    return false;
}

void Lv2Plugin::updateNativeGui()
{
    if (nativeGuiVisible())
    {
        updateUIPorts(true);

        if (ui.type == UI_EXTERNAL)
            LV2_EXTERNAL_UI_RUN((lv2_external_ui*)ui.widget);
    }
}

void Lv2Plugin::updateUIPorts(bool onlyOutput)
{
    if (ui.handle && ui.descriptor && ui.descriptor->port_event)
    {
        for (uint32_t i = 0; i < m_paramCount; i++)
        {
            if (m_params[i].type == PARAMETER_OUTPUT || onlyOutput == false || m_params[i].update)
                ui.descriptor->port_event(ui.handle, m_params[i].rindex, sizeof(float), 0, &m_paramsBuffer[i]);

            m_params[i].update = false;
        }
    }
}

void Lv2Plugin::closeNativeGui(bool destroyed)
{
#ifdef GTK2UI_SUPPORT
    if (ui.type == UI_GTK2 && destroyed == false)
    {
        GtkWidget* hostWidget   = (GtkWidget*)ui.nativeWidget;
        GtkWidget* pluginWidget = (GtkWidget*)ui.widget;
        gtk_container_remove(GTK_CONTAINER(hostWidget), pluginWidget);
        gtk_widget_destroy(hostWidget);
    }
#endif

    if (ui.descriptor->cleanup)
        ui.descriptor->cleanup(ui.handle);

    ui.handle = 0;
    ui.visible = false;
}

void Lv2Plugin::ui_resize(int width, int height)
{
    // We need to postpone this to the main event thread (send a signal to resize the UI?)
    if (ui.handle)
    {
        switch (ui.type)
        {
        case UI_GTK2:
            if (ui.nativeWidget)
                gtk_window_resize(GTK_WINDOW((GtkWidget*)ui.nativeWidget), width, height);
            break;
        case UI_QT4:
            if (ui.widget)
                ((QWidget*)ui.widget)->resize(width, height);
            break;
        case UI_X11:
            if (ui.nativeWidget)
                ((QWidget*)ui.nativeWidget)->resize(width, height);
            break;
        default:
            break;
        }
    }
}

void Lv2Plugin::ui_write_function(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
{
    if (format == 0) // control value changed
    {
        if (buffer_size == sizeof(float))
        {
            int32_t param_id = -1;
            float value = *(float*)buffer;

            for (uint32_t i=0; i < m_paramCount; i++)
            {
                if (m_params[i].rindex == (int32_t)port_index)
                {
                    param_id = i;
                    break;
                }
            }

            if (param_id > -1 && m_params[param_id].type == PARAMETER_INPUT)
            {
                m_params[param_id].value = m_params[param_id].tmpValue = m_paramsBuffer[param_id] = value;

                if (m_gui)
                    m_gui->setParameterValue(param_id, value);
            }
        }
    }
    // handle more formats later, not used in plugins currently
}

void Lv2Plugin::process(uint32_t frames, float** src, float** dst)
{
    if (descriptor && _enabled)
    {
        _proc_lock.lock();
        if (m_active)
        {
            // connect ports
            int ains  = m_audioInIndexes.size();
            int aouts = m_audioOutIndexes.size();
            bool need_buffer_copy  = false;
            bool need_extra_buffer = false;
            uint32_t pin, pout;

            if (ains == aouts)
            {
                if (aouts <= m_channels)
                {
                    for (int i=0; i < ains; i++)
                    {
                        pin  = m_audioInIndexes.at(i);
                        pout = m_audioOutIndexes.at(i);
                        descriptor->connect_port(handle, pin, src[i]);
                        descriptor->connect_port(handle, pout, dst[i]);
                        need_buffer_copy = true;
                    }
                }
                else
                {
                    for (int i=0; i < m_channels ; i++)
                    {
                        pin  = m_audioInIndexes.at(i);
                        pout = m_audioOutIndexes.at(i);
                        descriptor->connect_port(handle, pin, src[i]);
                        descriptor->connect_port(handle, pout, dst[i]);
                        need_extra_buffer = true;
                    }
                }
            }
            else
            {
                // cannot proccess
                _proc_lock.unlock();
                return;
            }

            // activate if needed
            if (m_activeBefore == false)
            {
                if (descriptor->activate)
                    descriptor->activate(handle);
            }

            // process
            if (need_extra_buffer)
            {
                if (m_hints & PLUGIN_HAS_IN_PLACE_BROKEN)
                {
                    // cannot proccess
                    _proc_lock.unlock();
                    return;
                }

                float extra_buffer[frames];
                memset(extra_buffer, 0, sizeof(float)*frames);

                for (int i=m_channels; i < ains ; i++)
                {
                    descriptor->connect_port(handle, m_audioInIndexes.at(i), extra_buffer);
                    descriptor->connect_port(handle, m_audioOutIndexes.at(i), extra_buffer);
                }
            }
            else
            {
                descriptor->run(handle, frames);

                if (need_buffer_copy)
                {
                    for (int i=ains; i < m_channels ; i++)
                        memcpy(dst[i], dst[i-1], sizeof(float)*frames);
                }
            }
        }
        else
        {
            // not active
            if (m_activeBefore)
            {
                if (descriptor->deactivate)
                    descriptor->deactivate(handle);
            }
        }
        m_activeBefore = m_active;
        _proc_lock.unlock();
    }
}

void Lv2Plugin::bufferSizeChanged(uint32_t)
{
}

bool Lv2Plugin::readConfiguration(Xml& xml, bool readPreset)
{
    QString new_uri;
    QString new_label;

    if (readPreset == false)
        m_channels = 1;

    for (;;)
    {
        Xml::Token token(xml.parse());
        const QString& tag(xml.s1());

        switch (token)
        {
            case Xml::Error:
            case Xml::End:
                return true;

            case Xml::TagStart:
                if (readPreset == false && ! _lib)
                {
                    LilvNode* pluginURI = lilv_new_uri(lv2world->world, new_uri.toAscii().constData());
                    lplug = lilv_plugins_get_by_uri(lv2world->plugins, pluginURI);
                    lilv_node_free(pluginURI);

                    new_label = QString(lilv_node_as_string(lilv_plugin_get_name(lplug)));

                    if (init(new_uri, new_label))
                    {
                        xml.parse1();
                        break;
                    }
                    else
                        return true;
                }

                if (tag == "control")
                {
                    loadControl(xml);
                }
                else if (tag == "active")
                {
                    if (readPreset == false)
                        m_active = xml.parseInt();
                }
                else if (tag == "gui")
                {
                    bool yesno = xml.parseInt();

                    if (yesno)
                    {
                        if (! m_gui)
                            m_gui = new PluginGui(this);
                        m_gui->show();
                    }
                }
                else if (tag == "geometry")
                {
                    QRect r(readGeometry(xml, tag));
                    if (m_gui)
                    {
                        m_gui->resize(r.size());
                        m_gui->move(r.topLeft());
                    }
                }
                else
                    xml.unknown("Lv2Plugin");

                break;

            case Xml::Attribut:
                if (tag == "uri")
                {
                    if (readPreset == false)
                        new_uri = xml.s2();
                }
                else if (tag == "channels")
                {
                    if (readPreset == false)
                        m_channels = xml.s2().toInt();
                }
                break;

            case Xml::TagEnd:
                if (tag == "Lv2Plugin")
                {
                    if (! _lib)
                        return true;

                    if (m_gui)
                        m_gui->updateValues();

                    return false;
                }
                return true;

            default:
                break;
        }
    }
    return true;
}

void Lv2Plugin::writeConfiguration(int level, Xml& xml)
{
    xml.tag(level++, "Lv2Plugin uri=\"%s\" channels=\"%d\"",
            Xml::xmlString(m_filename).toLatin1().constData(), m_channels);

    for (uint32_t i = 0; i < m_paramCount; i++)
    {
        double value = m_params[i].value;
        if (m_params[i].hints & PARAMETER_USES_SAMPLERATE)
            value /= sampleRate;

        QString s("control symbol=\"%1\" val=\"%2\" /");
        const LilvPort* port = lilv_plugin_get_port_by_index(lplug, m_params[i].rindex);
        xml.tag(level, s.arg(lilv_node_as_string(lilv_port_get_symbol(lplug, port))).arg(value, 0, 'f', 6).toLatin1().constData());
    }

    if ((m_hints & PLUGIN_HAS_EXTENSION_STATE) > 0 && descriptor->extension_data)
    {
        LV2_State_Interface* state = (LV2_State_Interface*)descriptor->extension_data(LV2_STATE_INTERFACE_URI);

        if (state)
        {
            state->save(handle, oom_lv2_state_store, this, 0, features);
        }
    }

    xml.intTag(level, "active", m_active);

    if (m_gui)
    {
        xml.intTag(level, "gui", 1);
        xml.geometryTag(level, "geometry", m_gui);
    }

    xml.tag(level--, "/Lv2Plugin");
}

bool Lv2Plugin::loadControl(Xml& xml)
{
    QString symbol;
    double val = 0.0;

    for (;;)
    {
        Xml::Token token = xml.parse();
        const QString& tag = xml.s1();

        switch (token)
        {
        case Xml::Error:
        case Xml::End:
            return true;

        case Xml::TagStart:
            xml.unknown("Lv2Plugin::control");
            break;

        case Xml::Attribut:
            if (tag == "symbol")
                symbol = xml.s2();
            else if (tag == "val")
                val = xml.s2().toDouble();
            break;

        case Xml::TagEnd:
            if (tag == "control" && symbol.isEmpty() == false && lplug)
                return setControl(symbol, val);
            return true;

        default:
            break;
        }
    }
    return true;
}

bool Lv2Plugin::setControl(QString symbol, double value)
{
    const LilvPort* port;
    for (uint32_t i = 0; i < m_paramCount; i++)
    {
        port = lilv_plugin_get_port_by_index(lplug, m_params[i].rindex);
        if (QString(lilv_node_as_string(lilv_port_get_symbol(lplug, port))) == symbol)
        {
            if (m_params[i].hints & PARAMETER_USES_SAMPLERATE)
                value *= sampleRate;
            m_params[i].value = m_params[i].tmpValue = m_paramsBuffer[i] = value;
            return false;
        }
    }
    return true;
}

#endif // __PLUGIN_LV2_H__