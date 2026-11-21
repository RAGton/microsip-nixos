/*
 * MicroSIP - Native Linux / NixOS Port (Qt 6 + PJSIP)
 * Based on MicroSIP (https://www.microsip.org)
 * Copyright (C) 2011-2026 MicroSIP (https://www.microsip.org)
 * Native Linux/NixOS Qt6 Port Copyright (C) 2026 Gabriel Aguiar Rocha (RAGton) <aguiarrocha37@outlook.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "SipEngine.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QRegularExpression>

namespace msip {

SipEngine* SipEngine::s_instance = nullptr;

static QString pjToQString(const pj_str_t& s) {
    if (!s.ptr || s.slen <= 0) return {};
    return QString::fromUtf8(s.ptr, static_cast<int>(s.slen));
}

SipEngine::SipEngine(Settings* settings, Database* db, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_db(db) {
    s_instance = this;
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(250);
    connect(m_pollTimer, &QTimer::timeout, this, &SipEngine::pollTimers);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(30000);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_running && !m_registered && m_accId != PJSUA_INVALID_ID) {
            emit logMessage("Auto-reconnect: refreshing SIP registration...");
            pjsua_acc_set_registration(m_accId, PJ_TRUE);
        }
    });
}

SipEngine::~SipEngine() {
    shutdown();
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool SipEngine::initialize() {
    if (m_running) return true;

    pj_status_t status = pjsua_create();
    if (status != PJ_SUCCESS) {
        emit logMessage("pjsua_create failed");
        return false;
    }

    pjsua_config uaCfg;
    pjsua_logging_config logCfg;
    pjsua_media_config mediaCfg;

    pjsua_config_default(&uaCfg);
    pjsua_logging_config_default(&logCfg);
    pjsua_media_config_default(&mediaCfg);

    uaCfg.max_calls = static_cast<unsigned>(qBound(1, m_settings->maxConcurrentCalls, PJSUA_MAX_CALLS));

    QByteArray uaBytes = m_settings->userAgent.toUtf8();
    uaCfg.user_agent = pj_str(uaBytes.data());

    uaCfg.cb.on_reg_state2 = &SipEngine::onRegState2;
    uaCfg.cb.on_incoming_call = &SipEngine::onIncomingCall;
    uaCfg.cb.on_call_state = &SipEngine::onCallState;
    uaCfg.cb.on_call_media_state = &SipEngine::onCallMediaState;
    uaCfg.cb.on_pager2 = &SipEngine::onPager2;
    uaCfg.cb.on_pager_status2 = &SipEngine::onPagerStatus2;
    uaCfg.cb.on_buddy_state = &SipEngine::onBuddyState;

    QByteArray stunBytes;
    if (m_settings->enableStun && !m_settings->stun.trimmed().isEmpty()) {
        stunBytes = m_settings->stun.trimmed().toUtf8();
        uaCfg.stun_srv_cnt = 1;
        uaCfg.stun_srv[0] = pj_str(stunBytes.data());
    }

    logCfg.level = m_settings->enableLog ? 4 : 2;
    logCfg.console_level = 2;
    QByteArray logPathBytes;
    if (m_settings->enableLog) {
        logPathBytes = m_settings->logFile.toUtf8();
        logCfg.log_filename = pj_str(logPathBytes.data());
    }

    mediaCfg.no_vad = m_settings->vad ? PJ_FALSE : PJ_TRUE;
    mediaCfg.ec_tail_len = m_settings->ec ? PJSUA_DEFAULT_EC_TAIL_LEN : 0;
    mediaCfg.clock_rate = 16000;
    mediaCfg.snd_auto_close_time = 1;

    status = pjsua_init(&uaCfg, &logCfg, &mediaCfg);
    if (status != PJ_SUCCESS) {
        emit logMessage("pjsua_init failed");
        pjsua_destroy();
        return false;
    }

    // UDP Transport
    {
        pjsua_transport_config tcfg;
        pjsua_transport_config_default(&tcfg);
        tcfg.port = static_cast<unsigned>(qMax(0, m_settings->sourcePort));
        status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &tcfg, &m_udpTransport);
        if (status != PJ_SUCCESS) {
            tcfg.port = 0;
            pjsua_transport_create(PJSIP_TRANSPORT_UDP, &tcfg, &m_udpTransport);
        }
    }

    // TCP Transport
    {
        pjsua_transport_config tcfg;
        pjsua_transport_config_default(&tcfg);
        tcfg.port = 0;
        pjsua_transport_create(PJSIP_TRANSPORT_TCP, &tcfg, &m_tcpTransport);
    }

    // TLS Transport (optional, depends on OpenSSL in pjproject)
#if defined(PJSIP_HAS_TLS_TRANSPORT) && PJSIP_HAS_TLS_TRANSPORT
    {
        pjsua_transport_config tcfg;
        pjsua_transport_config_default(&tcfg);
        tcfg.port = 0;
        tcfg.tls_setting.verify_server = PJ_FALSE;
        tcfg.tls_setting.verify_client = PJ_FALSE;
        pjsua_transport_create(PJSIP_TRANSPORT_TLS, &tcfg, &m_tlsTransport);
    }
#endif

    status = pjsua_start();
    if (status != PJ_SUCCESS) {
        emit logMessage("pjsua_start failed");
        pjsua_destroy();
        return false;
    }

    m_running = true;
    applyAudioDevices();
    applyAudioCodecs();
    setInputVolume(m_settings->volumeInput);
    setOutputVolume(m_settings->volumeOutput);

    m_pollTimer->start();
    m_reconnectTimer->start();
    emit logMessage("PJSIP initialized successfully");

    registerActiveAccount();
    return true;
}

void SipEngine::shutdown() {
    if (!m_running) return;
    m_pollTimer->stop();
    m_reconnectTimer->stop();
    stopRingtone();

    if (m_dtmfSlot != PJSUA_INVALID_ID) {
        pjsua_conf_disconnect(m_dtmfSlot, 0);
        pjsua_conf_remove_port(m_dtmfSlot);
        m_dtmfSlot = PJSUA_INVALID_ID;
    }
    if (m_dtmfPort) {
        pjmedia_port_destroy(m_dtmfPort);
        m_dtmfPort = nullptr;
    }
    if (m_dtmfPool) {
        pj_pool_release(m_dtmfPool);
        m_dtmfPool = nullptr;
    }

    for (auto it = m_calls.begin(); it != m_calls.end(); ++it) {
        if (it->recorderId != PJSUA_INVALID_ID) {
            pjsua_recorder_destroy(it->recorderId);
            it->recorderId = PJSUA_INVALID_ID;
        }
    }
    m_calls.clear();
    clearSubscriptions();

    if (m_accId != PJSUA_INVALID_ID) {
        pjsua_acc_del(m_accId);
        m_accId = PJSUA_INVALID_ID;
    }

    pjsua_call_hangup_all();
    pjsua_destroy();

    m_running = false;
    m_registered = false;
    m_regStatusText = "Offline";
    m_udpTransport = PJSUA_INVALID_ID;
    m_tcpTransport = PJSUA_INVALID_ID;
    m_tlsTransport = PJSUA_INVALID_ID;
}

bool SipEngine::restart() {
    shutdown();
    return initialize();
}

bool SipEngine::registerActiveAccount() {
    if (!m_running) return false;

    unregisterAccount();

    Account acc = m_settings->activeAccount();
    if (acc.isEmpty()) {
        m_registered = false;
        m_regStatusText = "No Account";
        emit registrationStateChanged(false, 0, m_regStatusText);
        return false;
    }

    pjsua_acc_config cfg;
    pjsua_acc_config_default(&cfg);

    QString domain = acc.domain.isEmpty() ? acc.server : acc.domain;
    QString server = acc.server.isEmpty() ? acc.domain : acc.server;
    QString transportSuffix;
    if (acc.transport.compare("tcp", Qt::CaseInsensitive) == 0) {
        transportSuffix = ";transport=tcp";
    } else if (acc.transport.compare("tls", Qt::CaseInsensitive) == 0) {
        transportSuffix = ";transport=tls";
    }

    QString idStr;
    if (!acc.displayName.isEmpty()) {
        idStr = QString("\"%1\" <sip:%2@%3>").arg(acc.displayName, acc.username, domain);
    } else {
        idStr = QString("sip:%1@%2").arg(acc.username, domain);
    }

    QString regUriStr = QString("sip:%1%2").arg(server, transportSuffix);
    if (acc.port > 0 && !server.contains(':')) {
        regUriStr = QString("sip:%1:%2%3").arg(server).arg(acc.port).arg(transportSuffix);
    }

    QByteArray idBytes = idStr.toUtf8();
    QByteArray regBytes = regUriStr.toUtf8();
    cfg.id = pj_str(idBytes.data());
    cfg.reg_uri = pj_str(regBytes.data());
    cfg.reg_timeout = static_cast<unsigned>(acc.registerRefresh > 0 ? acc.registerRefresh : 300);
    cfg.ka_interval = static_cast<unsigned>(qMax(0, acc.keepAlive));
    cfg.publish_enabled = acc.publish ? PJ_TRUE : PJ_FALSE;
    cfg.ice_cfg_use = PJSUA_ICE_CONFIG_USE_CUSTOM;
    cfg.ice_cfg.enable_ice = acc.ice ? PJ_TRUE : PJ_FALSE;
    cfg.allow_contact_rewrite = acc.allowRewrite ? PJ_TRUE : PJ_FALSE;

    if (acc.disableSessionTimer) {
        cfg.use_timer = PJSUA_SIP_TIMER_INACTIVE;
    }

    if (acc.srtp.compare("mandatory", Qt::CaseInsensitive) == 0) {
        cfg.use_srtp = PJMEDIA_SRTP_MANDATORY;
        cfg.srtp_secure_signaling = 0;
    } else if (acc.srtp.compare("optional", Qt::CaseInsensitive) == 0) {
        cfg.use_srtp = PJMEDIA_SRTP_OPTIONAL;
        cfg.srtp_secure_signaling = 0;
    } else {
        cfg.use_srtp = PJMEDIA_SRTP_DISABLED;
    }

    QByteArray proxyBytes;
    if (!acc.proxy.trimmed().isEmpty()) {
        QString p = acc.proxy.trimmed();
        if (!p.startsWith("sip:", Qt::CaseInsensitive) && !p.startsWith("sips:", Qt::CaseInsensitive)) {
            p = "sip:" + p;
        }
        if (!p.contains(";lr")) {
            p += ";lr";
        }
        if (!transportSuffix.isEmpty() && !p.contains("transport=", Qt::CaseInsensitive)) {
            p += transportSuffix;
        }
        proxyBytes = p.toUtf8();
        cfg.proxy_cnt = 1;
        cfg.proxy[0] = pj_str(proxyBytes.data());
    }

    QString authUser = acc.authID.isEmpty() ? acc.username : acc.authID;
    QByteArray realmBytes("*");
    QByteArray schemeBytes("digest");
    QByteArray userBytes = authUser.toUtf8();
    QByteArray passBytes = acc.password.toUtf8();

    cfg.cred_count = 1;
    cfg.cred_info[0].realm = pj_str(realmBytes.data());
    cfg.cred_info[0].scheme = pj_str(schemeBytes.data());
    cfg.cred_info[0].username = pj_str(userBytes.data());
    cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    cfg.cred_info[0].data = pj_str(passBytes.data());

    pj_status_t status = pjsua_acc_add(&cfg, PJ_TRUE, &m_accId);
    if (status != PJ_SUCCESS) {
        m_accId = PJSUA_INVALID_ID;
        m_registered = false;
        m_regStatusText = "Account Error";
        emit registrationStateChanged(false, status, m_regStatusText);
        return false;
    }

    m_regStatusText = "Connecting...";
    emit registrationStateChanged(false, 0, m_regStatusText);
    return true;
}

void SipEngine::unregisterAccount() {
    clearSubscriptions();
    if (m_running && m_accId != PJSUA_INVALID_ID) {
        pjsua_acc_del(m_accId);
        m_accId = PJSUA_INVALID_ID;
    }
    m_registered = false;
}

QVector<AudioDeviceInfo> SipEngine::audioDevices() const {
    QVector<AudioDeviceInfo> list;
    if (!m_running) return list;

    pjmedia_aud_dev_info infos[64];
    unsigned count = 64;
    if (pjsua_enum_aud_devs(infos, &count) == PJ_SUCCESS) {
        for (unsigned i = 0; i < count; ++i) {
            AudioDeviceInfo dev;
            dev.id = static_cast<int>(i);
            dev.name = QString::fromUtf8(infos[i].name);
            dev.driver = QString::fromUtf8(infos[i].driver);
            dev.inputCount = infos[i].input_count;
            dev.outputCount = infos[i].output_count;
            list.push_back(dev);
        }
    }
    return list;
}

void SipEngine::applyAudioDevices() {
    if (!m_running) return;
    int capDev = PJMEDIA_AUD_DEFAULT_CAPTURE_DEV;
    int playDev = PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV;

    auto devs = audioDevices();
    for (const auto& d : devs) {
        if (!m_settings->audioInputDevice.isEmpty() && d.inputCount > 0 && d.name == m_settings->audioInputDevice) {
            capDev = d.id;
        }
        if (!m_settings->audioOutputDevice.isEmpty() && d.outputCount > 0 && d.name == m_settings->audioOutputDevice) {
            playDev = d.id;
        }
    }
    pjsua_set_snd_dev(capDev, playDev);
}

QStringList SipEngine::availableAudioCodecs() const {
    QStringList list;
    if (!m_running) return list;
    pjsua_codec_info infos[64];
    unsigned count = 64;
    if (pjsua_enum_codecs(infos, &count) == PJ_SUCCESS) {
        for (unsigned i = 0; i < count; ++i) {
            list.append(pjToQString(infos[i].codec_id));
        }
    }
    return list;
}

void SipEngine::applyAudioCodecs() {
    if (!m_running) return;
    pjsua_codec_info infos[64];
    unsigned count = 64;
    if (pjsua_enum_codecs(infos, &count) != PJ_SUCCESS) return;

    QStringList enabled = m_settings->audioCodecs.split(' ', Qt::SkipEmptyParts);
    if (enabled.isEmpty()) return;

    for (unsigned i = 0; i < count; ++i) {
        QString cid = pjToQString(infos[i].codec_id);
        int idx = enabled.indexOf(cid);
        pj_uint8_t prio = 0;
        if (idx >= 0) {
            prio = static_cast<pj_uint8_t>(qMax(1, PJMEDIA_CODEC_PRIO_HIGHEST - idx));
        }
        pjsua_codec_set_priority(&infos[i].codec_id, prio);
    }
}

void SipEngine::setInputVolume(int percent) {
    m_settings->volumeInput = qBound(0, percent, 200);
    if (!m_running) return;
    float level = m_globalMuteInput ? 0.0f : (m_settings->volumeInput / 100.0f);
    pjsua_conf_adjust_rx_level(0, level);
}

void SipEngine::setOutputVolume(int percent) {
    m_settings->volumeOutput = qBound(0, percent, 200);
    if (!m_running) return;
    float level = m_globalMuteOutput ? 0.0f : (m_settings->volumeOutput / 100.0f);
    pjsua_conf_adjust_tx_level(0, level);
}

void SipEngine::setGlobalMuteInput(bool muted) {
    m_globalMuteInput = muted;
    setInputVolume(m_settings->volumeInput);
}

void SipEngine::setGlobalMuteOutput(bool muted) {
    m_globalMuteOutput = muted;
    setOutputVolume(m_settings->volumeOutput);
}

QString SipEngine::formatSipUri(const QString& input) const {
    QString s = input.trimmed();
    if (s.isEmpty()) return {};
    if (s.startsWith("sip:", Qt::CaseInsensitive) || s.startsWith("sips:", Qt::CaseInsensitive)) {
        return s;
    }

    Account acc = m_settings->activeAccount();
    if (!acc.dialingPrefix.isEmpty() && !s.contains('@') && !s.startsWith(acc.dialingPrefix)) {
        s = acc.dialingPrefix + s;
    }

    if (!s.contains('@')) {
        QString domain = acc.domain.isEmpty() ? acc.server : acc.domain;
        if (!domain.isEmpty()) {
            s = QString("sip:%1@%2").arg(s, domain);
        } else {
            s = "sip:" + s;
        }
    } else {
        s = "sip:" + s;
    }

    if (acc.transport.compare("tcp", Qt::CaseInsensitive) == 0 && !s.contains("transport=")) {
        s += ";transport=tcp";
    } else if (acc.transport.compare("tls", Qt::CaseInsensitive) == 0 && !s.contains("transport=")) {
        s += ";transport=tls";
    }
    return s;
}

QString SipEngine::extractNumberFromUri(const QString& uri) {
    QRegularExpression re(R"(sips?:([^@>;\s]+))");
    auto match = re.match(uri);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return uri;
}

QString SipEngine::extractDisplayNameFromUri(const QString& uri) {
    QRegularExpression re(R"(^\"([^\"]+)\"\s*<)");
    auto match = re.match(uri.trimmed());
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return {};
}

int SipEngine::makeCall(const QString& number) {
    if (!m_running || m_accId == PJSUA_INVALID_ID) return -1;
    QString uriStr = formatSipUri(number);
    if (uriStr.isEmpty()) return -1;

    if (m_settings->singleMode && !m_calls.isEmpty()) {
        hangupAll();
    }

    QByteArray uriBytes = uriStr.toUtf8();
    pj_str_t pjUri = pj_str(uriBytes.data());
    pjsua_call_id callId = PJSUA_INVALID_ID;
    pj_status_t status = pjsua_call_make_call(m_accId, &pjUri, nullptr, nullptr, nullptr, &callId);
    if (status != PJ_SUCCESS) {
        emit logMessage(QString("Failed to call %1 (status %2)").arg(uriStr).arg(status));
        return -1;
    }

    m_settings->lastCallNumber = number;
    m_settings->save();

    CallRuntime rt;
    rt.info.callId = callId;
    rt.info.remoteUri = uriStr;
    rt.info.number = extractNumberFromUri(uriStr);
    rt.info.incoming = false;
    rt.info.stateText = "Calling...";
    rt.startEpoch = QDateTime::currentSecsSinceEpoch();

    if (auto c = m_db->findContactByNumber(rt.info.number)) {
        rt.info.remoteName = c->name;
    }

    m_calls.insert(callId, rt);
    emit activeCallsChanged();
    return callId;
}

int SipEngine::callPickup(const QString& number) {
    if (number.trimmed().isEmpty()) return -1;
    QString prefix = m_settings->featureCodeCP.isEmpty() ? "**" : m_settings->featureCodeCP;
    return makeCall(prefix + number.trimmed());
}

bool SipEngine::answerCall(int callId) {
    if (!m_running || callId < 0) return false;
    stopRingtone();
    pj_status_t status = pjsua_call_answer(callId, 200, nullptr, nullptr);
    if (status == PJ_SUCCESS && m_calls.contains(callId)) {
        m_calls[callId].answered = true;
    }
    return status == PJ_SUCCESS;
}

bool SipEngine::answerFirstIncomingCall() {
    for (const auto& rt : m_calls) {
        if (rt.info.incoming && rt.info.state == PJSIP_INV_STATE_INCOMING) {
            return answerCall(rt.info.callId);
        }
    }
    return false;
}

void SipEngine::hangupCall(int callId) {
    if (!m_running || callId < 0) return;
    stopRecording(callId);
    pjsua_call_hangup(callId, 0, nullptr, nullptr);
    stopRingtoneIfNoRinging();
}

void SipEngine::rejectCall(int callId, int statusCode) {
    if (!m_running || callId < 0) return;
    pjsua_call_hangup(callId, static_cast<unsigned>(statusCode), nullptr, nullptr);
    stopRingtoneIfNoRinging();
}

void SipEngine::hangupAll() {
    if (!m_running) return;
    for (int id : m_calls.keys()) {
        stopRecording(id);
    }
    stopRingtone();
    pjsua_call_hangup_all();
}

bool SipEngine::toggleHold(int callId) {
    if (!m_running || !m_calls.contains(callId)) return false;
    auto& rt = m_calls[callId];
    pj_status_t status;
    if (rt.info.onHold) {
        status = pjsua_call_reinvite(callId, PJSUA_CALL_UNHOLD, nullptr);
        if (status == PJ_SUCCESS) rt.info.onHold = false;
    } else {
        status = pjsua_call_set_hold(callId, nullptr);
        if (status == PJ_SUCCESS) rt.info.onHold = true;
    }
    emit callStateChanged(rt.info);
    emit activeCallsChanged();
    return status == PJ_SUCCESS;
}

bool SipEngine::toggleMute(int callId) {
    if (!m_running || !m_calls.contains(callId)) return false;
    pjsua_call_info ci;
    if (pjsua_call_get_info(callId, &ci) != PJ_SUCCESS) return false;
    if (ci.conf_slot == PJSUA_INVALID_ID) return false;

    auto& rt = m_calls[callId];
    rt.info.muted = !rt.info.muted;
    if (rt.info.muted) {
        pjsua_conf_disconnect(0, ci.conf_slot);
    } else {
        pjsua_conf_connect(0, ci.conf_slot);
    }
    emit callStateChanged(rt.info);
    emit activeCallsChanged();
    return true;
}

bool SipEngine::sendDtmf(int callId, const QString& digits) {
    if (!m_running || callId < 0 || digits.isEmpty()) return false;
    QByteArray bytes = digits.toUtf8();
    pj_str_t pjDigits = pj_str(bytes.data());

    if (m_settings->dtmfMethod == DtmfMethod::SipInfo) {
        pjsua_call_send_dtmf_param param;
        pjsua_call_send_dtmf_param_default(&param);
        param.method = PJSUA_DTMF_METHOD_SIP_INFO;
        param.digits = pjDigits;
        return pjsua_call_send_dtmf(callId, &param) == PJ_SUCCESS;
    }

    pj_status_t st = pjsua_call_dial_dtmf(callId, &pjDigits);
    if (st != PJ_SUCCESS && m_settings->dtmfMethod == DtmfMethod::Auto) {
        pjsua_call_send_dtmf_param param;
        pjsua_call_send_dtmf_param_default(&param);
        param.method = PJSUA_DTMF_METHOD_SIP_INFO;
        param.digits = pjDigits;
        st = pjsua_call_send_dtmf(callId, &param);
    }
    return st == PJ_SUCCESS;
}

void SipEngine::playLocalDtmfDigit(char digit) {
    if (!m_running || !m_settings->localDtmf) return;
    if (!m_dtmfPool) {
        m_dtmfPool = pjsua_pool_create("dtmf", 4096, 4096);
        if (!m_dtmfPool) return;
        if (pjmedia_tonegen_create(m_dtmfPool, 16000, 1, 320, 16, 0, &m_dtmfPort) != PJ_SUCCESS) {
            pj_pool_release(m_dtmfPool);
            m_dtmfPool = nullptr;
            return;
        }
        if (pjsua_conf_add_port(m_dtmfPool, m_dtmfPort, &m_dtmfSlot) != PJ_SUCCESS) {
            pjmedia_port_destroy(m_dtmfPort);
            pj_pool_release(m_dtmfPool);
            m_dtmfPort = nullptr;
            m_dtmfPool = nullptr;
            return;
        }
        pjsua_conf_connect(m_dtmfSlot, 0);
    }

    pjmedia_tone_digit d;
    d.digit = digit;
    d.on_msec = 90;
    d.off_msec = 20;
    d.volume = 10000;
    pjmedia_tonegen_play_digits(m_dtmfPort, 1, &d, 0);
}

bool SipEngine::blindTransfer(int callId, const QString& destNumber) {
    if (!m_running || callId < 0 || destNumber.trimmed().isEmpty()) return false;
    QString uri = formatSipUri(destNumber);
    QByteArray bytes = uri.toUtf8();
    pj_str_t pjUri = pj_str(bytes.data());
    return pjsua_call_xfer(callId, &pjUri, nullptr) == PJ_SUCCESS;
}

bool SipEngine::attendedTransfer(int callId, int destCallId) {
    if (!m_running || callId < 0 || destCallId < 0 || callId == destCallId) return false;
    return pjsua_call_xfer_replaces(callId, destCallId, 0, nullptr) == PJ_SUCCESS;
}

bool SipEngine::forwardCall(int callId, const QString& destNumber) {
    if (!m_running || callId < 0 || destNumber.trimmed().isEmpty()) return false;
    QString uri = formatSipUri(destNumber);
    QByteArray bytes = uri.toUtf8();

    pjsua_msg_data msgData;
    pjsua_msg_data_init(&msgData);
    pj_pool_t* pool = pjsua_pool_create("fwd", 512, 512);
    if (!pool) return false;

    QByteArray hdrName("Contact");
    QByteArray hdrVal = "<" + bytes + ">";
    pj_str_t hname = pj_str(hdrName.data());
    pj_str_t hval = pj_str(hdrVal.data());
    pjsip_generic_string_hdr* hdr = pjsip_generic_string_hdr_create(pool, &hname, &hval);
    pj_list_push_back(&msgData.hdr_list, hdr);

    pj_status_t st = pjsua_call_hangup(callId, 302, nullptr, &msgData);
    pj_pool_release(pool);
    stopRingtoneIfNoRinging();
    return st == PJ_SUCCESS;
}

void SipEngine::joinConference() {
    if (!m_running) return;
    QList<int> ids = m_calls.keys();
    QList<pjsua_conf_port_id> slotsList;

    for (int id : ids) {
        if (m_calls[id].info.onHold) {
            pjsua_call_reinvite(id, PJSUA_CALL_UNHOLD, nullptr);
            m_calls[id].info.onHold = false;
        }
        pjsua_call_info ci;
        if (pjsua_call_get_info(id, &ci) == PJ_SUCCESS && ci.conf_slot != PJSUA_INVALID_ID) {
            slotsList.append(ci.conf_slot);
            m_calls[id].info.inConference = true;
        }
    }

    for (int i = 0; i < slotsList.size(); ++i) {
        for (int j = 0; j < slotsList.size(); ++j) {
            if (i != j) {
                pjsua_conf_connect(slotsList[i], slotsList[j]);
            }
        }
    }
    emit activeCallsChanged();
}

bool SipEngine::startRecording(int callId) {
    if (!m_running || !m_calls.contains(callId)) return false;
    auto& rt = m_calls[callId];
    if (rt.recorderId != PJSUA_INVALID_ID) return true;

    pjsua_call_info ci;
    if (pjsua_call_get_info(callId, &ci) != PJ_SUCCESS || ci.conf_slot == PJSUA_INVALID_ID) {
        return false;
    }

    QDir().mkpath(m_settings->recordingPath);
    QString safeNum = rt.info.number;
    safeNum.replace(QRegularExpression("[^a-zA-Z0-9_+.-]"), "_");
    QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    QString filePath = QString("%1/%2_%3.wav").arg(m_settings->recordingPath, stamp, safeNum);
    QByteArray pathBytes = filePath.toUtf8();
    pj_str_t pjPath = pj_str(pathBytes.data());

    pjsua_recorder_id recId = PJSUA_INVALID_ID;
    if (pjsua_recorder_create(&pjPath, 0, nullptr, 0, 0, &recId) != PJ_SUCCESS) {
        return false;
    }

    pjsua_conf_port_id recPort = pjsua_recorder_get_conf_port(recId);
    pjsua_conf_connect(0, recPort);
    pjsua_conf_connect(ci.conf_slot, recPort);

    rt.recorderId = recId;
    rt.recordingFile = filePath;
    rt.info.recording = true;

    emit recordingStateChanged(callId, true, filePath);
    emit callStateChanged(rt.info);
    emit activeCallsChanged();
    return true;
}

void SipEngine::stopRecording(int callId) {
    if (!m_running || !m_calls.contains(callId)) return;
    auto& rt = m_calls[callId];
    if (rt.recorderId == PJSUA_INVALID_ID) return;

    pjsua_recorder_destroy(rt.recorderId);
    rt.recorderId = PJSUA_INVALID_ID;
    rt.info.recording = false;

    emit recordingStateChanged(callId, false, rt.recordingFile);
    emit callStateChanged(rt.info);
    emit activeCallsChanged();
}

bool SipEngine::isRecording(int callId) const {
    return m_calls.contains(callId) && m_calls[callId].recorderId != PJSUA_INVALID_ID;
}

bool SipEngine::sendMessage(const QString& number, const QString& text) {
    if (!m_running || m_accId == PJSUA_INVALID_ID || number.trimmed().isEmpty() || text.isEmpty()) {
        return false;
    }
    QString uri = formatSipUri(number);
    QByteArray uriBytes = uri.toUtf8();
    QByteArray bodyBytes = text.toUtf8();
    pj_str_t pjUri = pj_str(uriBytes.data());
    pj_str_t pjBody = pj_str(bodyBytes.data());

    pj_status_t st = pjsua_im_send(m_accId, &pjUri, nullptr, &pjBody, nullptr, nullptr);
    if (st == PJ_SUCCESS) {
        SipMessage msg;
        msg.peerUri = extractNumberFromUri(uri);
        msg.peerName = msg.peerUri;
        if (auto c = m_db->findContactByNumber(msg.peerUri)) {
            msg.peerName = c->name;
        }
        msg.body = text;
        msg.timestamp = QDateTime::currentSecsSinceEpoch();
        msg.incoming = false;
        msg.status = "Sent";
        m_db->addMessage(msg);
        return true;
    }
    return false;
}

void SipEngine::clearSubscriptions() {
    if (m_running) {
        for (auto it = m_buddies.begin(); it != m_buddies.end(); ++it) {
            if (it.value() != PJSUA_INVALID_ID) {
                pjsua_buddy_del(it.value());
            }
        }
    }
    m_buddies.clear();
}

void SipEngine::subscribeUri(const QString& number) {
    if (!m_running || m_accId == PJSUA_INVALID_ID || number.trimmed().isEmpty()) return;
    QString cleanNum = extractNumberFromUri(number.trimmed());
    if (m_buddies.contains(cleanNum)) return;

    QString uri = formatSipUri(number);
    QByteArray uriBytes = uri.toUtf8();

    pjsua_buddy_config bcfg;
    pjsua_buddy_config_default(&bcfg);
    bcfg.uri = pj_str(uriBytes.data());
    bcfg.subscribe = PJ_TRUE;

    pjsua_buddy_id bid = PJSUA_INVALID_ID;
    if (pjsua_buddy_add(&bcfg, &bid) == PJ_SUCCESS) {
        m_buddies.insert(cleanNum, bid);
    }
}

void SipEngine::refreshSubscriptions(const QVector<Contact>& contacts, const QVector<Shortcut>& shortcuts) {
    clearSubscriptions();
    if (!m_running || !m_registered) return;

    for (const auto& c : contacts) {
        if (c.presence && !c.number.isEmpty()) {
            subscribeUri(c.number);
        }
    }
    for (const auto& s : shortcuts) {
        if (s.presence && !s.number.isEmpty()) {
            subscribeUri(s.number);
        }
    }
}

QVector<ActiveCallInfo> SipEngine::activeCalls() const {
    QVector<ActiveCallInfo> list;
    for (const auto& rt : m_calls) {
        list.push_back(rt.info);
    }
    return list;
}

int SipEngine::primaryCallId() const {
    if (m_calls.isEmpty()) return -1;
    // Prefer confirmed/unheld call first
    for (const auto& rt : m_calls) {
        if (rt.info.state == PJSIP_INV_STATE_CONFIRMED && !rt.info.onHold) {
            return rt.info.callId;
        }
    }
    return m_calls.begin().key();
}

void SipEngine::startRingtone() {
    if (!m_running) return;
    if (m_ringPlayerId != PJSUA_INVALID_ID || m_ringToneSlot != PJSUA_INVALID_ID) return;

    if (!m_settings->ringtone.isEmpty() && QFileInfo::exists(m_settings->ringtone)) {
        QByteArray pathBytes = m_settings->ringtone.toUtf8();
        pj_str_t pjPath = pj_str(pathBytes.data());
        if (pjsua_player_create(&pjPath, 0, &m_ringPlayerId) == PJ_SUCCESS) {
            pjsua_conf_port_id port = pjsua_player_get_conf_port(m_ringPlayerId);
            pjsua_conf_adjust_tx_level(port, m_settings->volumeRing / 100.0f);
            pjsua_conf_connect(port, 0);
            return;
        }
    }

    // Fallback synthesized ringback/ring tone via pjmedia tonegen
    m_ringPool = pjsua_pool_create("ringtone", 4096, 4096);
    if (!m_ringPool) return;

    if (pjmedia_tonegen_create2(m_ringPool, nullptr, 16000, 1, 320, 16, PJMEDIA_TONEGEN_LOOP, &m_ringTonePort) == PJ_SUCCESS) {
        pjmedia_tone_desc tones[2];
        pj_bzero(tones, sizeof(tones));
        tones[0].freq1 = 440;
        tones[0].freq2 = 480;
        tones[0].on_msec = 1200;
        tones[0].off_msec = 2400;
        tones[0].volume = static_cast<short>(12000 * (m_settings->volumeRing / 100.0f));

        pjmedia_tonegen_play(m_ringTonePort, 1, tones, PJMEDIA_TONEGEN_LOOP);
        if (pjsua_conf_add_port(m_ringPool, m_ringTonePort, &m_ringToneSlot) == PJ_SUCCESS) {
            pjsua_conf_connect(m_ringToneSlot, 0);
        }
    }
}

void SipEngine::stopRingtone() {
    if (m_ringPlayerId != PJSUA_INVALID_ID) {
        pjsua_player_destroy(m_ringPlayerId);
        m_ringPlayerId = PJSUA_INVALID_ID;
    }
    if (m_ringToneSlot != PJSUA_INVALID_ID) {
        pjsua_conf_disconnect(m_ringToneSlot, 0);
        pjsua_conf_remove_port(m_ringToneSlot);
        m_ringToneSlot = PJSUA_INVALID_ID;
    }
    if (m_ringTonePort) {
        pjmedia_port_destroy(m_ringTonePort);
        m_ringTonePort = nullptr;
    }
    if (m_ringPool) {
        pj_pool_release(m_ringPool);
        m_ringPool = nullptr;
    }
}

void SipEngine::stopRingtoneIfNoRinging() {
    for (const auto& rt : m_calls) {
        if (rt.info.incoming && rt.info.state == PJSIP_INV_STATE_INCOMING) {
            return;
        }
    }
    stopRingtone();
}

void SipEngine::pollTimers() {
    if (!m_running) return;

    int64_t now = QDateTime::currentSecsSinceEpoch();
    bool changed = false;
    for (auto it = m_calls.begin(); it != m_calls.end(); ++it) {
        if (it->connectEpoch > 0) {
            int dur = static_cast<int>(now - it->connectEpoch);
            if (dur != it->info.duration) {
                it->info.duration = dur;
                changed = true;
            }
            if (m_settings->autoHangUpTime > 0 && dur >= m_settings->autoHangUpTime) {
                hangupCall(it.key());
            }
        }
    }
    if (changed) {
        emit activeCallsChanged();
    }

    if (!m_calls.isEmpty()) {
        unsigned tx = 0, rx = 0;
        if (pjsua_conf_get_signal_level(0, &tx, &rx) == PJ_SUCCESS) {
            m_lastLevelsActive = true;
            emit audioLevelsUpdated(tx / 255.0f, rx / 255.0f);
        }
    } else if (m_lastLevelsActive) {
        m_lastLevelsActive = false;
        emit audioLevelsUpdated(0.0f, 0.0f);
    }
}

// --- Static PJSUA Callbacks ---

void SipEngine::onRegState2(pjsua_acc_id acc_id, pjsua_reg_info* info) {
    Q_UNUSED(acc_id);
    if (!s_instance || !info || !info->cbparam) return;
    int code = info->cbparam->code;
    QString reason = pjToQString(info->cbparam->reason);
    bool ok = (code >= 200 && code < 300 && info->cbparam->expiration > 0);

    QMetaObject::invokeMethod(s_instance, [ok, code, reason]() {
        if (s_instance) s_instance->handleRegState(ok, code, reason);
    }, Qt::QueuedConnection);
}

void SipEngine::onIncomingCall(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data* rdata) {
    Q_UNUSED(acc_id);
    Q_UNUSED(rdata);
    if (!s_instance) return;

    pjsua_call_info ci;
    if (pjsua_call_get_info(call_id, &ci) != PJ_SUCCESS) return;

    QString remoteUri = pjToQString(ci.remote_info);
    QString sipCallId = pjToQString(ci.call_id);

    QMetaObject::invokeMethod(s_instance, [call_id, remoteUri, sipCallId]() {
        if (s_instance) s_instance->handleIncomingCall(call_id, remoteUri, sipCallId);
    }, Qt::QueuedConnection);
}

void SipEngine::onCallState(pjsua_call_id call_id, pjsip_event* e) {
    Q_UNUSED(e);
    if (!s_instance) return;

    pjsua_call_info ci;
    if (pjsua_call_get_info(call_id, &ci) != PJ_SUCCESS) return;

    int state = static_cast<int>(ci.state);
    QString stateText = pjToQString(ci.state_text);
    QString remoteUri = pjToQString(ci.remote_info);
    QString sipCallId = pjToQString(ci.call_id);
    int lastStatus = static_cast<int>(ci.last_status);
    QString lastStatusText = pjToQString(ci.last_status_text);

    QMetaObject::invokeMethod(s_instance, [call_id, state, stateText, remoteUri, sipCallId, lastStatus, lastStatusText]() {
        if (s_instance) {
            s_instance->handleCallState(call_id, state, stateText, remoteUri, sipCallId, lastStatus, lastStatusText);
        }
    }, Qt::QueuedConnection);
}

void SipEngine::onCallMediaState(pjsua_call_id call_id) {
    if (!s_instance) return;
    QMetaObject::invokeMethod(s_instance, [call_id]() {
        if (s_instance) s_instance->handleCallMediaState(call_id);
    }, Qt::QueuedConnection);
}

void SipEngine::onPager2(pjsua_call_id call_id, const pj_str_t* from, const pj_str_t* to,
                         const pj_str_t* contact, const pj_str_t* mime_type,
                         const pj_str_t* body, pjsip_rx_data* rdata, pjsua_acc_id acc_id) {
    Q_UNUSED(call_id); Q_UNUSED(to); Q_UNUSED(contact); Q_UNUSED(mime_type); Q_UNUSED(rdata); Q_UNUSED(acc_id);
    if (!s_instance || !from || !body) return;
    QString fromStr = pjToQString(*from);
    QString bodyStr = pjToQString(*body);
    QMetaObject::invokeMethod(s_instance, [fromStr, bodyStr]() {
        if (s_instance) s_instance->handlePager(fromStr, bodyStr);
    }, Qt::QueuedConnection);
}

void SipEngine::onPagerStatus2(pjsua_call_id call_id, const pj_str_t* to, const pj_str_t* body,
                               void* user_data, pjsip_status_code status, const pj_str_t* reason,
                               pjsip_tx_data* tdata, pjsip_rx_data* rdata, pjsua_acc_id acc_id) {
    Q_UNUSED(call_id); Q_UNUSED(body); Q_UNUSED(user_data); Q_UNUSED(tdata); Q_UNUSED(rdata); Q_UNUSED(acc_id);
    if (!s_instance || !to) return;
    QString peer = extractNumberFromUri(pjToQString(*to));
    int code = static_cast<int>(status);
    QString reasonStr = reason ? pjToQString(*reason) : QString();
    QMetaObject::invokeMethod(s_instance, [peer, code, reasonStr]() {
        if (s_instance) emit s_instance->messageStatusChanged(peer, code, reasonStr);
    }, Qt::QueuedConnection);
}

void SipEngine::onBuddyState(pjsua_buddy_id buddy_id) {
    if (!s_instance) return;
    pjsua_buddy_info bi;
    if (pjsua_buddy_get_info(buddy_id, &bi) != PJ_SUCCESS) return;

    QString uri = pjToQString(bi.uri);
    QString statusText = pjToQString(bi.status_text);
    PresenceStatus ps = PresenceStatus::Unknown;
    bool ringing = false;

    if (bi.status == PJSUA_BUDDY_STATUS_ONLINE) {
        ps = PresenceStatus::Online;
        if (bi.rpid.activity == PJRPID_ACTIVITY_BUSY) {
            ps = PresenceStatus::Busy;
        } else if (bi.rpid.activity == PJRPID_ACTIVITY_AWAY) {
            ps = PresenceStatus::Away;
        }
        if (statusText.contains("ring", Qt::CaseInsensitive) || statusText.contains("early", Qt::CaseInsensitive)) {
            ps = PresenceStatus::Ringing;
            ringing = true;
        } else if (statusText.contains("phone", Qt::CaseInsensitive) || statusText.contains("talk", Qt::CaseInsensitive) || statusText.contains("busy", Qt::CaseInsensitive)) {
            ps = PresenceStatus::OnThePhone;
        }
    } else if (bi.status == PJSUA_BUDDY_STATUS_OFFLINE) {
        ps = PresenceStatus::Offline;
    }

    QMetaObject::invokeMethod(s_instance, [uri, ps, ringing, statusText]() {
        if (s_instance) s_instance->handleBuddyState(uri, ps, ringing, statusText);
    }, Qt::QueuedConnection);
}

// --- GUI-thread Handlers ---

void SipEngine::handleRegState(bool registered, int code, const QString& reason) {
    m_registered = registered;
    m_regStatusText = registered ? "Online" : (reason.isEmpty() ? QString("Error %1").arg(code) : reason);
    emit registrationStateChanged(registered, code, m_regStatusText);
    emit logMessage(QString("Registration: %1 (%2 %3)").arg(registered ? "Online" : "Offline").arg(code).arg(reason));
}

void SipEngine::handleIncomingCall(int callId, const QString& remoteUri, const QString& sipCallId) {
    // DND check
    if (m_settings->dnd) {
        pjsua_call_hangup(callId, 486, nullptr, nullptr);
        CallRecord rec;
        rec.callId = sipCallId;
        rec.number = extractNumberFromUri(remoteUri);
        rec.name = extractDisplayNameFromUri(remoteUri);
        rec.type = CallDirection::Missed;
        rec.time = QDateTime::currentSecsSinceEpoch();
        rec.info = "DND (486 Busy)";
        m_db->addCall(rec);
        emit callEnded(callId, rec);
        return;
    }

    // Call Waiting / Single mode check
    if ((!m_settings->callWaiting || m_settings->singleMode) && !m_calls.isEmpty()) {
        pjsua_call_hangup(callId, 486, nullptr, nullptr);
        return;
    }

    // Immediate Call Forwarding check
    if (m_settings->fwd && !m_settings->forwardingNumber.isEmpty() && m_settings->forwardingDelay <= 0) {
        forwardCall(callId, m_settings->forwardingNumber);
        return;
    }

    CallRuntime rt;
    rt.info.callId = callId;
    rt.info.sipCallId = sipCallId;
    rt.info.remoteUri = remoteUri;
    rt.info.number = extractNumberFromUri(remoteUri);
    rt.info.remoteName = extractDisplayNameFromUri(remoteUri);
    if (auto c = m_db->findContactByNumber(rt.info.number)) {
        if (!c->name.isEmpty()) rt.info.remoteName = c->name;
    }
    rt.info.incoming = true;
    rt.info.state = PJSIP_INV_STATE_INCOMING;
    rt.info.stateText = "Incoming";
    rt.startEpoch = QDateTime::currentSecsSinceEpoch();
    m_calls.insert(callId, rt);

    pjsua_call_answer(callId, 180, nullptr, nullptr);
    startRingtone();

    emit incomingCall(rt.info);
    emit activeCallsChanged();

    // Delayed Call Forwarding
    if (m_settings->fwd && !m_settings->forwardingNumber.isEmpty() && m_settings->forwardingDelay > 0) {
        QTimer::singleShot(m_settings->forwardingDelay * 1000, this, [this, callId]() {
            if (m_calls.contains(callId) && m_calls[callId].info.state == PJSIP_INV_STATE_INCOMING) {
                forwardCall(callId, m_settings->forwardingNumber);
            }
        });
    }

    // Auto-Answer (AA)
    bool shouldAA = m_settings->aa || m_settings->autoAnswer == "all";
    if (shouldAA && !m_settings->autoAnswerNumber.isEmpty()) {
        shouldAA = rt.info.number.contains(m_settings->autoAnswerNumber, Qt::CaseInsensitive);
    }
    if (shouldAA) {
        int delayMs = qMax(0, m_settings->autoAnswerDelay) * 1000;
        QTimer::singleShot(delayMs, this, [this, callId]() {
            if (m_calls.contains(callId) && m_calls[callId].info.state == PJSIP_INV_STATE_INCOMING) {
                answerCall(callId);
            }
        });
    }
}

void SipEngine::handleCallState(int callId, int state, const QString& stateText,
                                const QString& remoteUri, const QString& sipCallId,
                                int lastStatus, const QString& lastStatusText) {
    if (!m_calls.contains(callId)) {
        if (state == PJSIP_INV_STATE_DISCONNECTED) return;
        CallRuntime rt;
        rt.info.callId = callId;
        rt.info.sipCallId = sipCallId;
        rt.info.remoteUri = remoteUri;
        rt.info.number = extractNumberFromUri(remoteUri);
        rt.startEpoch = QDateTime::currentSecsSinceEpoch();
        m_calls.insert(callId, rt);
    }

    auto& rt = m_calls[callId];
    rt.info.state = state;
    rt.info.stateText = stateText;
    if (rt.info.sipCallId.isEmpty()) rt.info.sipCallId = sipCallId;

    if (state == PJSIP_INV_STATE_CONFIRMED) {
        stopRingtoneIfNoRinging();
        if (rt.connectEpoch == 0) {
            rt.connectEpoch = QDateTime::currentSecsSinceEpoch();
        }
        rt.answered = true;
    }

    emit callStateChanged(rt.info);

    if (state == PJSIP_INV_STATE_DISCONNECTED) {
        stopRecording(callId);

        CallRecord rec;
        rec.callId = rt.info.sipCallId.isEmpty() ? QString::number(rt.startEpoch) : rt.info.sipCallId;
        rec.number = rt.info.number;
        rec.name = rt.info.remoteName;
        rec.time = rt.startEpoch > 0 ? rt.startEpoch : QDateTime::currentSecsSinceEpoch();
        rec.duration = rt.connectEpoch > 0 ? static_cast<int>(QDateTime::currentSecsSinceEpoch() - rt.connectEpoch) : 0;
        if (rt.info.incoming) {
            rec.type = rt.answered ? CallDirection::Incoming : CallDirection::Missed;
        } else {
            rec.type = CallDirection::Outgoing;
        }
        rec.info = lastStatus > 0 ? QString("%1 %2").arg(lastStatus).arg(lastStatusText).trimmed() : stateText;

        rec.id = m_db->addCall(rec);
        m_calls.remove(callId);
        stopRingtoneIfNoRinging();

        emit callEnded(callId, rec);
        emit activeCallsChanged();
        return;
    }

    emit activeCallsChanged();
}

void SipEngine::handleCallMediaState(int callId) {
    if (!m_running) return;
    pjsua_call_info ci;
    if (pjsua_call_get_info(callId, &ci) != PJ_SUCCESS) return;

    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE || ci.media_status == PJSUA_CALL_MEDIA_REMOTE_HOLD) {
        if (ci.conf_slot != PJSUA_INVALID_ID) {
            pjsua_conf_connect(ci.conf_slot, 0);
            if (!m_calls.contains(callId) || !m_calls[callId].info.muted) {
                pjsua_conf_connect(0, ci.conf_slot);
            }
            if (m_calls.contains(callId) && m_settings->autoRecording && !m_calls[callId].info.recording) {
                startRecording(callId);
            }
        }
    }
    if (m_calls.contains(callId)) {
        m_calls[callId].info.onHold = (ci.media_status == PJSUA_CALL_MEDIA_LOCAL_HOLD);
        pjmedia_transport_info tpInfo;
        pjmedia_transport_info_init(&tpInfo);
        if (pjsua_call_get_med_transport_info(callId, 0, &tpInfo) == PJ_SUCCESS) {
            for (unsigned i = 0; i < tpInfo.specific_info_cnt; ++i) {
                if (tpInfo.spc_info[i].type == PJMEDIA_TRANSPORT_TYPE_SRTP) {
                    auto* srtpInfo = reinterpret_cast<const pjmedia_srtp_info*>(tpInfo.spc_info[i].buffer);
                    m_calls[callId].info.srtpActive = (srtpInfo->active != PJ_FALSE);
                    break;
                }
            }
        }
        emit callStateChanged(m_calls[callId].info);
        emit activeCallsChanged();
    }
}

void SipEngine::handlePager(const QString& fromUri, const QString& body) {
    SipMessage msg;
    msg.peerUri = extractNumberFromUri(fromUri);
    msg.peerName = extractDisplayNameFromUri(fromUri);
    if (auto c = m_db->findContactByNumber(msg.peerUri)) {
        if (!c->name.isEmpty()) msg.peerName = c->name;
    }
    if (msg.peerName.isEmpty()) msg.peerName = msg.peerUri;
    msg.body = body;
    msg.timestamp = QDateTime::currentSecsSinceEpoch();
    msg.incoming = true;
    msg.status = "Received";

    msg.id = m_db->addMessage(msg);
    emit messageReceived(msg);
}

void SipEngine::handleBuddyState(const QString& uri, PresenceStatus status, bool ringing, const QString& statusText) {
    QString num = extractNumberFromUri(uri);
    emit buddyStateChanged(num, status, ringing, statusText);
}

} // namespace msip
