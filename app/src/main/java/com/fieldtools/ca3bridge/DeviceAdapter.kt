package com.fieldtools.ca3bridge

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView

class DeviceAdapter(
    private val devices: List<UsbDevice>,
    private val context: Context,
    private val onClick: (UsbDevice) -> Unit
) : RecyclerView.Adapter<DeviceAdapter.ViewHolder>() {
    
    class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        val vidPid: TextView = view.findViewById(R.id.deviceVidPid)
        val deviceClass: TextView = view.findViewById(R.id.deviceClass)
        val product: TextView = view.findViewById(R.id.deviceProduct)
        val manufacturer: TextView = view.findViewById(R.id.deviceManufacturer)
        val interfaces: TextView = view.findViewById(R.id.deviceInterfaces)
        val selected: TextView = view.findViewById(R.id.deviceSelected)
    }
    
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context).inflate(R.layout.item_usb_device, parent, false)
        return ViewHolder(view)
    }
    
    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val device = devices[position]
        holder.vidPid.text = String.format("0x%04X:0x%04X", device.vendorId, device.productId)
        holder.deviceClass.text = String.format("Class: 0x%02X Sub: 0x%02X Proto: 0x%02X",
            device.deviceClass, device.deviceSubclass, device.deviceProtocol)
        holder.product.text = device.productName ?: "Unknown product"
        holder.manufacturer.text = device.manufacturerName ?: "Unknown manufacturer"
        holder.interfaces.text = "Interfaces: ${device.interfaceCount}"
        
        val isSelected = device.deviceId == DeviceAdapter.selectedDeviceId
        holder.selected.visibility = if (isSelected) View.VISIBLE else View.GONE
        holder.itemView.setBackgroundColor(
            if (isSelected) ContextCompat.getColor(context, R.color.slate_700)
            else ContextCompat.getColor(context, R.color.slate_900)
        )
        
        holder.itemView.setOnClickListener { onClick(device) }
    }
    
    override fun getItemCount() = devices.size
    
    companion object {
        var selectedDeviceId: Int = -1
    }
}