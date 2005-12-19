#include <dr-env.h>
#include <devices.h>

#include "../../drivers/pci/pci.h"

int pcitest_init(int argc, char *argv[])
{
  int hdev, devid;
  fd32_request_t *pci_request;
  pci_request_t pcirq;
  pci_device *pcidev;
  pcirq.size = sizeof(pci_request_t);
  pcirq.base_class = 0x01;
  pcirq.sub_class = 0x01;
  pcirq.pcidev_obj = &pcidev;
  
  hdev = fd32_dev_search("pci");
  if (hdev >= 0 && fd32_dev_get(hdev, &pci_request, NULL, NULL, 0) >= 0)
  {
    if ((devid = pci_request(FD32_PCI_GET_DEVICE, &pcirq)) >= 0)
    {
    /* read PCI config */
      fd32_message("vender: %d\n", pcidev->pci_dv_vendor);
    /* write PCI config */
      
    }
  }
  return 0;
}
