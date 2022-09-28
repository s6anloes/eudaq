#include <queue>
#include <cmath>

#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Utils.hh"

struct drpixel{
  uint8_t board_id;
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
  std::vector<int> Filling_Channel_Map(eudaq::ConfigurationSPC conf) const;
  std::vector<int> Filling_Pedestals(eudaq::ConfigurationSPC conf) const;
  std::priority_queue<drpixel> Filling_Priority_Queue(const std::vector<uint8_t> &data0, eudaq::ConfigurationSPC conf) const;
  virtual eudaq::StandardPlane Filling_Plane(eudaq::StandardEventSP d2, std::priority_queue<drpixel> pq, std::vector<int> channel_map, uint8_t board_id) const;

  static const uint32_t m_id_factory = eudaq::cstr2hash("DualROCaloEvent");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<DualROCaloRawEvent2StdEventConverter>(DualROCaloRawEvent2StdEventConverter::m_id_factory);
}

std::vector<int> DualROCaloRawEvent2StdEventConverter::Filling_Channel_Map(eudaq::ConfigurationSPC conf) const{

  auto map_file = conf->Get("map_file", "default_map_file.txt");

  std::string content;
  content = eudaq::ReadLineFromFile(map_file);
  std::vector<int> channel_map;
  std::stringstream ss(content);
  for (int i; ss >> i;) {
    channel_map.push_back(i);    
    if (ss.peek() == ',') ss.ignore();
  }

  return channel_map;

}


std::vector<int> DualROCaloRawEvent2StdEventConverter::Filling_Pedestals(eudaq::ConfigurationSPC conf) const{
  
  auto hg_pedestal_file = conf->Get("hg_pedestal_file", "default_hg_pedestal_file.txt");

  std::string content;
  content = eudaq::ReadLineFromFile(hg_pedestal_file);
  std::vector<int> pedestals;
  std::stringstream ss2(content);

  float j;

  while (ss2 >> j)
    {
        pedestals.push_back(std::round(j));

        if (ss2.peek() == ',')
        ss2.ignore();
    }
  /*
    for (int k=0; k< hg_pedestals.size(); k++)
    std::cout << hg_pedestals.at(k)<<std::endl;
  */

  return pedestals;

}


std::priority_queue<drpixel> DualROCaloRawEvent2StdEventConverter::Filling_Priority_Queue(const std::vector<uint8_t> &data0, eudaq::ConfigurationSPC conf) const{
  auto send_hg_value = conf->Get("send_hg_value", false);

  std::vector<int> channel_map = DualROCaloRawEvent2StdEventConverter::Filling_Channel_Map(conf);
  std::vector<int> pedestals = DualROCaloRawEvent2StdEventConverter::Filling_Pedestals(conf);

  std::priority_queue<drpixel> pq;


  uint8_t board_id = data0[0];
  datait it0 = data0.begin()+1; // +1 because first entry in block is board_id

  while (it0 < data0.end()) {
    
    uint8_t channel_id = eudaq::getlittleendian<uint8_t>(&(*(it0)));
    uint16_t lg_adc_value = eudaq::getlittleendian<uint16_t>(&(*(it0+2)));
    uint16_t hg_adc_value = eudaq::getlittleendian<uint16_t>(&(*(it0+4)));

    uint8_t n = channel_map[channel_id];
    int16_t ped_subtracted_hg = hg_adc_value - pedestals[n+64*board_id];
    //std::cout << "PEDESTAL is = " << std::to_string(pedestals[n+64*board_id]) << std::endl;


    drpixel thispixel = {board_id, channel_id, lg_adc_value, ped_subtracted_hg};
    pq.push(thispixel);

    it0 += 6;
    
  }

  return pq;
}

eudaq::StandardPlane DualROCaloRawEvent2StdEventConverter::Filling_Plane(eudaq::StandardEventSP d2, std::priority_queue<drpixel> pq, std::vector<int> channel_map, uint8_t board_id) const{

  // Identify the detetor type
  d2->SetDetectorType("DualROCalo");

  eudaq::StandardPlane plane(0, "DualROCalo", "DualROCalo");
  plane.SetSizeZS(16, 20, 0);

  for (int p=0; p<1; p++){
    uint8_t n = channel_map[pq.top().channel_id];
    uint16_t x = n % 16;
    uint16_t y = 19 - ((uint16_t) n/16 + 4*board_id); //channel numbering starts from the top (19 because we start from 0)
    plane.PushPixel(x, y, pq.top().hg_adc_value);
    //std::cout<<"DualROCaloRAWEventConverter:: Pushing Pixel with hg_adc_value = " << std::to_string(pq.top().hg_adc_value) << std::endl;
    pq.pop();
  }

  return plane;

}

bool DualROCaloRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{

  auto use_timestamps = conf->Get("use_timestamps", true);

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if(!ev)
    return false;


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
  uint8_t board_id = data0[0];

  std::vector<int> channel_map = DualROCaloRawEvent2StdEventConverter::Filling_Channel_Map(conf);
  std::vector<int> pedestals = DualROCaloRawEvent2StdEventConverter::Filling_Pedestals(conf);
  std::priority_queue<drpixel> pq = DualROCaloRawEvent2StdEventConverter::Filling_Priority_Queue(data0, conf);
  eudaq::StandardPlane plane = DualROCaloRawEvent2StdEventConverter::Filling_Plane(d2, pq, channel_map, board_id);
  
  d2->AddPlane(plane);
  //eudaq::mSleep(1000);

  return true;
}

