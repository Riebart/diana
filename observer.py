

class Observable:
    def __init__(self):
        self.observers = []

    def notify(self):
        for observer in observers:
            self.send_message(observer)
            
    def send_state(self, observer):
        pass
    
    def add_observer(self, observer):
        self.observers.append(observer)
        
    def remove_observer(self, observer):
        if observer in self.observers:
            self.observers.remove(observer)
            
    def notify_once(self, client):
        send_state(self, client)
        
        