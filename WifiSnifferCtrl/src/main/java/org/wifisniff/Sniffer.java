package org.wifisniff;

import org.wifisniff.pipes.NamedPipeWrapper;

import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.util.List;

public class Sniffer extends JFrame {


    private JComboBox<String> comPorts;
    private JButton btnConnect;
    private JPanel mainPanel;
    private JLabel lblAddr2;
    private JTextField txtAddr2;
    private JButton btnSend;

    private SerialCtrl serialCtrl;
    public Sniffer() {
        setContentPane(mainPanel);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(300, 400);
        setVisible(true);
        final List<String> allPorts = SerialCtrl.getAllPorts();
        for (String allPort : allPorts) {
            comPorts.addItem(allPort);
        }


        btnConnect.addActionListener(e -> {
            final Object selectedItem = comPorts.getSelectedItem().toString();

            String pipeName = "\\\\.\\pipe\\wireshark";


//            final NamedPipeWrapper hNamedPipe=new NamedPipeWrapper(pipeName);
//            final int speed =115200; //250000;//912600;
//            serialCtrl=new SerialCtrl(selectedItem.toString(), speed);
//            try {
//                new ProcessBuilder().command("C:\\Program Files\\Wireshark\\Wireshark.exe", "-i" + pipeName, "-k").start();
//                final boolean b =hNamedPipe.connect();
//                serialCtrl.open();
//                serialCtrl.send("OB");
//                serialCtrl.send("S1");
//                serialCtrl.send("F2");
//            } catch (IOException ex) {
//                throw new RuntimeException(ex);
//            }

        });
        btnSend.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                serialCtrl.send("F2"+txtAddr2);
            }
        });
    }

}
