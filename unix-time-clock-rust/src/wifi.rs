use esp_idf_hal::modem::Modem;
use esp_idf_svc::eventloop::EspSystemEventLoop;
use esp_idf_sys::esp;
use esp_idf_sys::{esp_wifi_set_ps, wifi_ps_type_t_WIFI_PS_NONE};

use esp_idf_svc::nvs::EspDefaultNvsPartition;
use esp_idf_svc::wifi::{BlockingWifi, EspWifi};
use esp_idf_svc::wifi::{ClientConfiguration, Configuration};
use esp_idf_sys::EspError;

pub(crate) fn configure(
    ssid: &str,
    pass: &str,
    modem: Modem,
) -> Result<BlockingWifi<EspWifi<'static>>, EspError> {
    // Configure Wifi
    let sysloop = EspSystemEventLoop::take()?;
    // The nvs stores the RF calibration data, which allows
    // for faster connection
    let nvs = EspDefaultNvsPartition::take()?;

    let mut wifi = BlockingWifi::wrap(EspWifi::new(modem, sysloop.clone(), Some(nvs))?, sysloop)?;

    wifi.set_configuration(&Configuration::Client(ClientConfiguration {
        ssid: ssid.try_into().unwrap(),
        password: pass.try_into().unwrap(),
        ..Default::default()
    }))?;

    wifi.start()?;
    // disable radio power saving; makes connectivity generally faster
    esp!(unsafe { esp_wifi_set_ps(wifi_ps_type_t_WIFI_PS_NONE) })?;
    wifi.connect()?;

    // Wait until the network interface is up
    wifi.wait_netif_up()?;

    let ip_info = wifi.wifi().sta_netif().get_ip_info()?;
    println!("IP info: {:?}", ip_info);
    Ok(wifi)
}
