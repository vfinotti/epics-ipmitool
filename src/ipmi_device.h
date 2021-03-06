/* SPDX-License-Identifier: MIT */
#ifndef IPMI_DEVICE_H_
#define IPMI_DEVICE_H_

extern "C" {
struct ipmi_intf;
struct sdr_record_compact_sensor;
struct sdr_record_full_sensor;
struct sensor_reading;
} // extern "C"

#include <callback.h>
#include <epicsMutex.h>

#include <cstdint>
#include <map>
#include <string>
#include <set>

#include "ipmi_types.h"

struct aiRecord;
struct dbCommon;
struct link;
struct mbbiDirectRecord;
struct mbbiRecord;

namespace IPMIIOC {

class ReaderThread;

class Device {
  public:
    Device(short _id);
    ~Device();

    bool connect(const std::string& _hostname, const std::string& _username,
                 const std::string& _password, const std::string& _proto, int _privlevel);

    void initAiRecord(::aiRecord* _rec);
    /** Update the record with information from the SDR. */
    void fillAiRecord(const any_record_ptr &_rec, const any_sensor_ptr &_sdr);
    bool readAiSensor(::aiRecord* _rec);

    void initMbbiDirectRecord(::mbbiDirectRecord* _rec);
    bool readMbbiDirectSensor(::mbbiDirectRecord* _rec);

    void initMbbiRecord(::mbbiRecord* _rec);
    /** Update the record with information from the SDR. */
    void fillMbbiRecord(const any_record_ptr &_rec, const any_sensor_ptr &_i);
    bool readMbbiSensor(::mbbiRecord* _rec);

    /** Full scan, all IPMBs we can find. */
    void detectSensors();
    void dumpDatabase(const std::string& _file);
    /** Smart scan: Only IPMBs with defined PVs. */
    void scanActiveIPMBs();

    bool ping();

  private:
    friend class ReaderThread;

    typedef bool (Device::*query_func_t)(const sensor_id_t&, result_t&);

    // description of a job queued for reading.
    // sensor id and function to call for it.
    // also the id of the PV that owns the job.
    struct query_job_t {
      sensor_id_t sensor;
      query_func_t query_func;
      unsigned pvid;

      query_job_t(const ::link& _loc, query_func_t _f, unsigned _pvid);
    };

    void handleFullSensor(slave_addr_t _addr, ::sdr_record_full_sensor* _rec);
    void handleCompactSensor(slave_addr_t _addr, ::sdr_record_compact_sensor* _rec);

    static void aiCallback(::CALLBACK* _cb);
    bool aiQuery(const sensor_id_t& _sensor, result_t& _result);

    static void mbbiDirectCallback(::CALLBACK* _cb);
    bool mbbiDirectQuery(const sensor_id_t& _sensor, result_t& _result);

    static void mbbiCallback(::CALLBACK* _cb);
    bool mbbiQuery(const sensor_id_t& _sensor, result_t& _result);

    const ::sensor_reading* ipmiQuery(const sensor_id_t& _sensor);

    bool check_PICMG();
    void find_ipmb();
    void iterateSDRs(slave_addr_t _addr, bool _force_internal = false);

    ::ipmi_intf* intf_ = nullptr;

    status_map_t sensors_;

    void initInputRecord(::dbCommon* _rec, const ::link& _inp);
    void initRecordDesc(const any_record_ptr& _rec, const any_sensor_ptr& _sdr);

    short id_;
    ::epicsMutex mutex_;
    ReaderThread* readerThread_;
    uint8_t local_addr_;

    /// used during full scan.
    std::set<slave_addr_t> slaves_;
    /// IPMBs with at least one defined PV.
    std::set<slave_addr_t> active_ipmbs_;

    using PVs_for_sensor_map_t = std::multimap<sensor_id_t, any_record_ptr>;
    /// sensor id-to-PV mapping
    PVs_for_sensor_map_t pv_map_;

    /** Functions to call for a given record type. */
    struct FunctionTable {
       void (Device::*fill)(const any_record_ptr &, const any_sensor_ptr &);
    };

    static std::map<RecordType, FunctionTable> record_functions_;
    /** Find PVs defined for an SDR and fill in their metadata. */
    void fillPVsFromSDR(const sensor_id_t &_id, const any_sensor_ptr &_sdr);
}; // class Device

} // namespace IPMIIOC

#endif

