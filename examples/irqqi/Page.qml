import QtQuick 1.0
import QtDesktop 0.1

Tab {
    id: tab

    function receiveMessage(message) {
        if (textEdit.text.length > 0)
            textEdit.text += "\n";
        textEdit.text += MessageFormatter.formatMessage(message);
    }

    Column {
        anchors.fill: parent
        ScrollArea {
            id: scrollArea
            width: parent.width
            height: parent.height - textField.height
            contentHeight: textEdit.height
            horizontalScrollBar.visible: false
            TextEdit {
                id: textEdit
                readOnly: true
                width: scrollArea.viewportWidth
                wrapMode: Text.Wrap
            }
        }
        TextField {
            id: textField
            width: parent.width
            Keys.onReturnPressed: {
                var cmd = command.createMessage(tab.title, textField.text);
                session.sendCommand(cmd);
                cmd.destroy();
                textField.text = "";
            }
        }
    }

    /*Rectangle {
        id: scrollbar
        anchors.right: flickable.right
        y: flickable.visibleArea.yPosition * flickable.height
        width: 2
        height: flickable.visibleArea.heightRatio * flickable.height
        color: "black"
    }*/
}
