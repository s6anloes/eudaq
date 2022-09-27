#include <queue>
#include <cmath>

#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Utils.hh"

struct drpixel{
  uint8_t channel_id;
  uint16_t lg_adc_value;
  int16_t hg_adc_value;

  bool operator<(const drpixel &o) const{
    return hg_adc_value < o.hg_adc_value; 
  }
};

class DualROCaloRawEvent2StdEventConverter: public eudaq::StdEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("DualROCaloEvent");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<DualROCaloRawEvent2StdEventConverter>(DualROCaloRawEvent2StdEventConverter::m_id_factory);
}

bool DualROCaloRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{


  auto map_file = conf->Get("map_file", "default_map_file.txt");
  auto hg_pedestal_file = conf->Get("hg_pedestal_file", "default_hg_pedestal_file.txt");
  auto use_timestamps = conf->Get("use_timestamps", true);
  auto send_hg_value = conf->Get("send_hg_value", false);

  std::string content;
  content = eudaq::ReadLineFromFile(map_file);
  std::vector<int> channel_map;
  std::stringstream ss(content);
  for (int i; ss >> i;) {
    channel_map.push_back(i);    
    if (ss.peek() == ',') ss.ignore();
  }

  
  content = eudaq::ReadLineFromFile(hg_pedestal_file);
  std::vector<int> hg_pedestals;
  std::stringstream ss2(content);

  float j;

  while (ss2 >> j)
    {
        hg_pedestals.push_back(std::round(j));

        if (ss2.peek() == ',')
        ss2.ignore();
    }
  /*
    for (int k=0; k< hg_pedestals.size(); k++)
    std::cout << hg_pedestals.at(k)<<std::endl;
  */

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if(!ev)
    return false;

  // Identify the detetor type
  d2->SetDetectorType("DualROCalo");


  //d2->SetTimeBegin(d1->GetTimestampBegin());
  //d2->SetTimeEnd(d1->GetTimestampEnd());

  if(!d2->IsFlagPacket()){
    d2->SetFlag(d1->GetFlag());
    d2->SetRunN(d1->GetRunN());
    d2->SetEventN(d1->GetEventN());
    d2->SetStreamN(d1->GetStreamN());
    if (d1->IsFlagTrigger()){
      d2->SetTriggerN(d1->GetTriggerN(), d1->IsFlagTrigger());
    }
    if (d1->IsFlagTimestamp() && use_timestamps){
      d2->SetTimestamp(d1->GetTimestampBegin()*1000000, d1->GetTimestampEnd()*1000000, d1->IsFlagTimestamp());
    }
  }

  auto &rawev = *ev;


  const std::vector<uint8_t> &data0 = rawev.GetBlock(0);
  datait it0 = data0.begin()+1; // +1 because first entry in block is board_id
  
  eudaq::StandardPlane plane(0, "DualROCalo", "DualROCalo");
  plane.SetSizeZS(16, 20, 0);

  eudaq::StandardPlane plane_lg(1, "DualROCalo", "DualROCalo");
  plane_lg.SetSizeZS(16, 20, 0);
  uint8_t board_id = data0[0];

  std::priority_queue<drpixel> pq;

  while (it0 < data0.end()) {
    
    uint8_t channel_id = eudaq::getlittleendian<uint8_t>(&(*(it0)));
    uint16_t lg_adc_value = eudaq::getlittleendian<uint16_t>(&(*(it0+2)));
    uint16_t hg_adc_value = eudaq::getlittleendian<uint16_t>(&(*(it0+4)));

    uint8_t n = channel_map[channel_id];
    int16_t ped_subtracted_hg = hg_adc_value - hg_pedestals[n+64*board_id];
    //std::cout << "PEDESTAL is = " << std::to_string(hg_pedestals[n+64*board_id]) << std::endl;


    drpixel thispixel = {channel_id, lg_adc_value, ped_subtracted_hg};
    pq.push(thispixel);

    /*
    uint8_t n = channel_map[channel_id];
    uint16_t x = n % 16;
    uint16_t y = 19 - ((uint16_t) n/16 + 4*board_id); //channel numbering starts from the top (19 because we start from 0)

    //std::cout << "Converting:: lg value = " << std::to_string(lg_adc_value) << std::endl;
    //std::cout<<"Converting:: n = " << std::to_string(n)<<" x = " << std::to_string(x)<<" y = " << std::to_string(y) << " adc = " << std::to_string(lg_adc_value) <<  std::endl;

  
    if (hg_adc_value>70) 
      plane.PushPixel(x, y, hg_adc_value);

    if (lg_adc_value>70)
      plane_lg.PushPixel(x, y, lg_adc_value);


    
    //plane.SetPivotPixel((9216 + pivot + PIVOTPIXELOFFSET) % 9216);
    //DecodeFrame(plane, 0, &it0[8], len0, use_all_hits);
    //DecodeFrame(plane, 1, &it1[8], len1, use_all_hits);
    */

    it0 += 6;
    
  }

  for (int p=0; p<2; p++){
    uint8_t n = channel_map[pq.top().channel_id];
    uint16_t x = n % 16;
    uint16_t y = 19 - ((uint16_t) n/16 + 4*board_id); //channel numbering starts from the top (19 because we start from 0)
    //if (send_hg_value)
      plane.PushPixel(x, y, pq.top().hg_adc_value);
    //else
      plane_lg.PushPixel(x, y, pq.top().lg_adc_value);
    //std::cout<<"DualROCaloRAWEventConverter:: Pushing Pixel with hg_adc_value = " << std::to_string(pq.top().hg_adc_value) << "and pedestal = << " << std::to_string(hg_pedestals[n+64*board_id]) << std::endl;
    pq.pop();
  }

  d2->AddPlane(plane);
  //eudaq::mSleep(1000);

  return true;
}

